/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "argus/lowlevel/atomic.hpp"
#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/math.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/canvas.hpp"
#include "argus/render/common/material.hpp"
#include "argus/render/defines.hpp"

#include "internal/render_opengl/defines.hpp"
#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/renderer/compositing.hpp"
#include "internal/render_opengl/renderer/shader_mgmt.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"
#include "internal/render_opengl/state/scene_state.hpp"
#include "internal/render_opengl/state/viewport_state.hpp"

#include "aglet/aglet.h"

#include <map>
#include <string>
#include <utility>

#include <climits>
#include <argus/render/2d/scene_2d.hpp>

#define BINDING_INDEX_VBO 0

namespace argus {
    struct TransformedViewport {
        int32_t top;
        int32_t bottom;
        int32_t left;
        int32_t right;
    };

    static TransformedViewport _transform_viewport_to_pixels(const Viewport &viewport, const Vector2u &resolution) {
        float vp_h_scale;
        float vp_v_scale;
        float vp_h_off;
        float vp_v_off;

        auto min_dim = float(std::min(resolution.x, resolution.y));
        auto max_dim = float(std::max(resolution.x, resolution.y));
        switch (viewport.mode) {
            case ViewportCoordinateSpaceMode::Individual:
                vp_h_scale = float(resolution.x);
                vp_v_scale = float(resolution.y);
                vp_h_off = 0;
                vp_v_off = 0;
                break;
            case ViewportCoordinateSpaceMode::MinAxis:
                vp_h_scale = min_dim;
                vp_v_scale = min_dim;
                vp_h_off = resolution.x > resolution.y ? float(resolution.x - resolution.y) / 2.0f : 0;
                vp_v_off = resolution.y > resolution.x ? float(resolution.y - resolution.x) / 2.0f : 0;
                break;
            case ViewportCoordinateSpaceMode::MaxAxis:
                vp_h_scale = max_dim;
                vp_v_scale = max_dim;
                vp_h_off = resolution.x < resolution.y ? -float(resolution.y - resolution.x) / 2.0f : 0;
                vp_v_off = resolution.y < resolution.x ? -float(resolution.x - resolution.y) / 2.0f : 0;
                break;
            case ViewportCoordinateSpaceMode::HorizontalAxis:
                vp_h_scale = float(resolution.x);
                vp_v_scale = float(resolution.x);
                vp_h_off = 0;
                vp_v_off = (float(resolution.y) - float(resolution.x)) / 2.0f;
                break;
            case ViewportCoordinateSpaceMode::VerticalAxis:
                vp_h_scale = float(resolution.y);
                vp_v_scale = float(resolution.y);
                vp_h_off = (float(resolution.x) - float(resolution.y)) / 2.0f;
                vp_v_off = 0;
                break;
            default:
                throw std::invalid_argument("Viewport mode is invalid");
        }

        TransformedViewport transformed{};
        transformed.left = int32_t(viewport.left * vp_h_scale + vp_h_off);
        transformed.right = int32_t(viewport.right * vp_h_scale + vp_h_off);
        transformed.top = int32_t(viewport.top * vp_v_scale + vp_v_off);
        transformed.bottom = int32_t(viewport.bottom * vp_v_scale + vp_v_off);

        return transformed;
    }

    static void _update_scene_ubo(SceneState &scene_state) {
        if (scene_state.scene.type == SceneType::TwoD) {
            auto &scene = reinterpret_cast<Scene2D &>(scene_state.scene);
            auto al_level = scene.get_ambient_light_level();
            auto al_color = scene.get_ambient_light_color();

            bool must_update = al_level.dirty || al_color.dirty;

            if (!scene_state.ubo.valid) {
                scene_state.ubo = BufferInfo::create(GL_UNIFORM_BUFFER, SHADER_UBO_SCENE_LEN, GL_STATIC_DRAW, false);
                must_update = true;
            }

            if (must_update) {
                if (al_level.dirty) {
                    scene_state.ubo.write_val<float>(al_level.value, SHADER_UNIFORM_SCENE_AL_LEVEL_OFF);
                }

                if (al_color.dirty) {
                    float color[4] = { al_color.value.r, al_color.value.g, al_color.value.b, 1.0 };
                    scene_state.ubo.write(color, sizeof(color), SHADER_UNIFORM_SCENE_AL_COLOR_OFF);
                }
            }
        }
    }

    static void _update_viewport_ubo(ViewportState &viewport_state) {
        bool must_update = viewport_state.view_matrix_dirty;

        if (!viewport_state.ubo.valid) {
            viewport_state.ubo = BufferInfo::create(GL_UNIFORM_BUFFER, SHADER_UBO_VIEWPORT_LEN, GL_STATIC_DRAW, false);
            must_update = true;
        }

        if (must_update) {
            viewport_state.ubo.write(viewport_state.view_matrix.data, sizeof(viewport_state.view_matrix.data),
                    SHADER_UNIFORM_VIEWPORT_VM_OFF);
        }
    }

    static void _bind_ubo(const LinkedProgram &program, const std::string &name, const BufferInfo &buffer) {
        program.reflection.get_ubo_binding_and_then(name, [&buffer](auto binding) {
            affirm_precond(binding <= INT_MAX, "UBO binding is too big");
            glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer.handle);
        });
    }

    void draw_scene_to_framebuffer(SceneState &scene_state, ViewportState &viewport_state,
            ValueAndDirtyFlag<Vector2u> resolution) {
        auto &state = scene_state.parent_state;

        auto viewport = viewport_state.viewport->get_viewport();
        auto viewport_px = _transform_viewport_to_pixels(viewport, resolution.value);

        auto fb_width = std::abs(viewport_px.right - viewport_px.left);
        auto fb_height = std::abs(viewport_px.bottom - viewport_px.top);

        // set scene uniforms
        _update_scene_ubo(scene_state);

        // set viewport uniforms
        _update_viewport_ubo(viewport_state);

        // framebuffer setup
        if (viewport_state.front_fb == 0) {
            if (AGLET_GL_ARB_direct_state_access) {
                glCreateFramebuffers(1, &viewport_state.front_fb);
                glCreateFramebuffers(1, &viewport_state.back_fb);
            } else {
                glGenFramebuffers(1, &viewport_state.front_fb);
                glGenFramebuffers(1, &viewport_state.back_fb);
            }
        }

        if (viewport_state.front_frame_tex == 0 || resolution.dirty) {
            if (viewport_state.front_frame_tex != 0) {
                glDeleteTextures(1, &viewport_state.front_frame_tex);
            }

            if (viewport_state.back_frame_tex != 0) {
                glDeleteTextures(1, &viewport_state.back_frame_tex);
            }

            if (AGLET_GL_ARB_direct_state_access) {
                glCreateTextures(GL_TEXTURE_2D, 1, &viewport_state.front_frame_tex);
                glCreateTextures(GL_TEXTURE_2D, 1, &viewport_state.back_frame_tex);

                glTextureStorage2D(viewport_state.front_frame_tex, 1, GL_RGBA8, fb_width, fb_height);
                glTextureStorage2D(viewport_state.back_frame_tex, 1, GL_RGBA8, fb_width, fb_height);

                glTextureParameteri(viewport_state.front_frame_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTextureParameteri(viewport_state.front_frame_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glTextureParameteri(viewport_state.back_frame_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTextureParameteri(viewport_state.back_frame_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glNamedFramebufferTexture(viewport_state.front_fb, GL_COLOR_ATTACHMENT0,
                        viewport_state.front_frame_tex, 0);
                glNamedFramebufferTexture(viewport_state.back_fb, GL_COLOR_ATTACHMENT0,
                        viewport_state.back_frame_tex, 0);

                auto front_fb_status = glCheckNamedFramebufferStatus(viewport_state.front_fb, GL_FRAMEBUFFER);
                if (front_fb_status != GL_FRAMEBUFFER_COMPLETE) {
                    Logger::default_logger().fatal("Front framebuffer is incomplete (error %d)", front_fb_status);
                }

                auto back_fb_status = glCheckNamedFramebufferStatus(viewport_state.back_fb, GL_FRAMEBUFFER);
                if (back_fb_status != GL_FRAMEBUFFER_COMPLETE) {
                    Logger::default_logger().fatal("Back framebuffer is incomplete (error %d)", back_fb_status);
                }
            } else {
                // back framebuffer texture
                glGenTextures(1, &viewport_state.back_frame_tex);
                glBindTexture(GL_TEXTURE_2D, viewport_state.back_frame_tex);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb_width, fb_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.back_fb);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                        viewport_state.back_frame_tex, 0);

                glBindTexture(GL_TEXTURE_2D, 0);

                auto back_fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (back_fb_status != GL_FRAMEBUFFER_COMPLETE) {
                    Logger::default_logger().fatal("Back framebuffer is incomplete (error %d)", back_fb_status);
                }

                // front framebuffer texture
                glGenTextures(1, &viewport_state.front_frame_tex);
                glBindTexture(GL_TEXTURE_2D, viewport_state.front_frame_tex);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb_width, fb_height, 0,
                        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.front_fb);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                        viewport_state.front_frame_tex, 0);

                glBindTexture(GL_TEXTURE_2D, 0);

                auto front_fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (front_fb_status != GL_FRAMEBUFFER_COMPLETE) {
                    Logger::default_logger().fatal("Front framebuffer is incomplete (error %d)", front_fb_status);
                }
            }
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.front_fb);

        // clear framebuffer
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        affirm_precond(resolution->x <= INT_MAX && resolution->y <= INT_MAX, "Resolution is too big for glViewport");

        glViewport(
                -viewport_px.left,
                -viewport_px.top,
                GLsizei(resolution->x),
                GLsizei(resolution->y)
        );

        program_handle_t last_program = 0;
        texture_handle_t last_texture = 0;

        for (auto &bucket : scene_state.render_buckets) {
            auto &mat = bucket.second->material_res;
            auto &program_info = state.linked_programs.find(mat.uid)->second;
            auto &texture_uid = mat.get<Material>().get_texture_uid();
            auto tex_handle = state.prepared_textures.find(texture_uid)->second;

            bool animated = program_info.reflection.has_ubo(SHADER_UBO_OBJ);

            if (program_info.handle != last_program) {
                glUseProgram(program_info.handle);
                last_program = program_info.handle;

                _bind_ubo(program_info, SHADER_UBO_GLOBAL, state.global_ubo);
                _bind_ubo(program_info, SHADER_UBO_SCENE, scene_state.ubo);
                _bind_ubo(program_info, SHADER_UBO_VIEWPORT, viewport_state.ubo);
            }

            if (animated) {
                _bind_ubo(program_info, SHADER_UBO_OBJ, bucket.second->obj_ubo);
            }

            if (tex_handle != last_texture) {
                glBindTexture(GL_TEXTURE_2D, tex_handle);
                last_texture = tex_handle;
            }

            glBindVertexArray(bucket.second->vertex_array);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glDrawArrays(GL_TRIANGLES, 0, GLsizei(bucket.second->vertex_count));

            glBindVertexArray(0);
        }

        if (!AGLET_GL_ARB_direct_state_access) {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        for (auto &postfx : viewport_state.viewport->get_postprocessing_shaders()) {
            LinkedProgram *postfx_program;

            auto &postfx_programs = state.postfx_programs;
            auto it = postfx_programs.find(postfx);
            if (it != postfx_programs.end()) {
                postfx_program = &it->second;
            } else {
                auto linked_program = link_program({FB_SHADER_VERT_PATH, postfx});
                auto inserted = postfx_programs.insert({postfx, linked_program});
                postfx_program = &inserted.first->second;
            }

            std::swap(viewport_state.front_fb, viewport_state.back_fb);
            std::swap(viewport_state.front_frame_tex, viewport_state.back_frame_tex);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.front_fb);

            glClearColor(0.0, 0.0, 0.0, 0.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(
                    0,
                    0,
                    fb_width,
                    fb_height
            );

            glBindVertexArray(state.frame_vao);
            glUseProgram(postfx_program->handle);
            glBindTexture(GL_TEXTURE_2D, viewport_state.back_frame_tex);

            _bind_ubo(*postfx_program, SHADER_UBO_GLOBAL, state.global_ubo);
            _bind_ubo(*postfx_program, SHADER_UBO_SCENE, scene_state.ubo);
            _bind_ubo(*postfx_program, SHADER_UBO_VIEWPORT, viewport_state.ubo);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        glBindVertexArray(0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void draw_framebuffer_to_screen(SceneState &scene_state, ViewportState &viewport_state,
            ValueAndDirtyFlag<Vector2u> resolution) {
        auto &state = scene_state.parent_state;

        auto viewport_px = _transform_viewport_to_pixels(viewport_state.viewport->get_viewport(), resolution.value);
        auto viewport_width_px = std::abs(viewport_px.right - viewport_px.left);
        auto viewport_height_px = std::abs(viewport_px.bottom - viewport_px.top);

        auto viewport_y = GLsizei(resolution->y) - viewport_px.bottom;
        affirm_precond(resolution->y <= INT_MAX && viewport_y <= INT_MAX, "Viewport Y is too big for glViewport");

        glViewport(
                viewport_px.left,
                GLsizei(viewport_y),
                viewport_width_px,
                viewport_height_px
        );

        glBindVertexArray(state.frame_vao);
        glUseProgram(state.frame_program.value().handle);
        glBindTexture(GL_TEXTURE_2D, viewport_state.front_frame_tex);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        glBindVertexArray(0);
    }

    void setup_framebuffer(RendererState &state) {
        auto frame_program = link_program({FB_SHADER_VERT_PATH, FB_SHADER_FRAG_PATH});

        state.frame_program = frame_program;

        if (!frame_program.reflection.get_attr_loc(SHADER_ATTRIB_POSITION).has_value()) {
            Logger::default_logger().fatal("Frame program is missing required position attribute");
        }
        if (!frame_program.reflection.get_attr_loc(SHADER_ATTRIB_TEXCOORD).has_value()) {
            Logger::default_logger().fatal("Frame program is missing required texcoords attribute");
        }

        float frame_quad_vertex_data[] = {
                -1.0, -1.0, 0.0, 0.0,
                -1.0, 1.0, 0.0, 1.0,
                1.0, 1.0, 1.0, 1.0,
                -1.0, -1.0, 0.0, 0.0,
                1.0, 1.0, 1.0, 1.0,
                1.0, -1.0, 1.0, 0.0,
        };

        if (AGLET_GL_ARB_direct_state_access) {
            glCreateVertexArrays(1, &state.frame_vao);

            glCreateBuffers(1, &state.frame_vbo);

            glNamedBufferData(state.frame_vbo, sizeof(frame_quad_vertex_data), frame_quad_vertex_data, GL_STATIC_DRAW);

            glVertexArrayVertexBuffer(state.frame_vao, BINDING_INDEX_VBO, state.frame_vbo, 0,
                    4 * uint32_t(sizeof(GLfloat)));
        } else {
            glGenVertexArrays(1, &state.frame_vao);
            glBindVertexArray(state.frame_vao);

            glGenBuffers(1, &state.frame_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, state.frame_vbo);

            glBufferData(GL_ARRAY_BUFFER, sizeof(frame_quad_vertex_data), frame_quad_vertex_data, GL_STATIC_DRAW);
        }

        unsigned int attr_offset = 0;
        set_attrib_pointer(state.frame_vao, state.frame_vbo, BINDING_INDEX_VBO, 4, SHADER_ATTRIB_POSITION_LEN,
                FB_SHADER_ATTRIB_POSITION_LOC, &attr_offset);
        set_attrib_pointer(state.frame_vao, state.frame_vbo, BINDING_INDEX_VBO, 4, SHADER_ATTRIB_TEXCOORD_LEN,
                FB_SHADER_ATTRIB_TEXCOORD_LOC, &attr_offset);

        if (!AGLET_GL_ARB_direct_state_access) {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    }
}
