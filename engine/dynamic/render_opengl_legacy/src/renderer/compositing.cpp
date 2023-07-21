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

#include "internal/render_opengl_legacy/defines.hpp"
#include "internal/render_opengl_legacy/types.hpp"
#include "internal/render_opengl_legacy/gl_util.hpp"
#include "internal/render_opengl_legacy/renderer/bucket_proc.hpp"
#include "internal/render_opengl_legacy/renderer/compositing.hpp"
#include "internal/render_opengl_legacy/renderer/shader_mgmt.hpp"
#include "internal/render_opengl_legacy/state/render_bucket.hpp"
#include "internal/render_opengl_legacy/state/renderer_state.hpp"
#include "internal/render_opengl_legacy/state/scene_state.hpp"
#include "internal/render_opengl_legacy/state/viewport_state.hpp"

#include "aglet/aglet.h"

#include <map>
#include <string>
#include <utility>

#include <climits>

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
                assert(false);
        }

        TransformedViewport transformed{};
        transformed.left = int32_t(viewport.left * vp_h_scale + vp_h_off);
        transformed.right = int32_t(viewport.right * vp_h_scale + vp_h_off);
        transformed.top = int32_t(viewport.top * vp_v_scale + vp_v_off);
        transformed.bottom = int32_t(viewport.bottom * vp_v_scale + vp_v_off);

        return transformed;
    }

    static void _set_fb_attribs(buffer_handle_t frame_vbo) {
        unsigned int attr_offset = 0;
        set_attrib_pointer(frame_vbo, 4, SHADER_ATTRIB_POSITION_LEN,
                FB_SHADER_ATTRIB_POSITION_LOC, &attr_offset);
        set_attrib_pointer(frame_vbo, 4, SHADER_ATTRIB_TEXCOORD_LEN,
                FB_SHADER_ATTRIB_TEXCOORD_LOC, &attr_offset);
    }

    void draw_scene_to_framebuffer(SceneState &scene_state, ViewportState &viewport_state,
            ValueAndDirtyFlag<Vector2u> resolution) {
        auto &state = scene_state.parent_state;

        auto viewport = viewport_state.viewport->get_viewport();
        auto viewport_px = _transform_viewport_to_pixels(viewport, resolution.value);

        auto fb_width = std::abs(viewport_px.right - viewport_px.left);
        auto fb_height = std::abs(viewport_px.bottom - viewport_px.top);

        // framebuffer setup
        if (viewport_state.front_fb == 0) {
            glGenFramebuffersEXT(1, &viewport_state.front_fb);
            glGenFramebuffersEXT(1, &viewport_state.back_fb);
        }

        if (viewport_state.front_frame_tex == 0 || resolution.dirty) {
            if (viewport_state.front_frame_tex != 0) {
                glDeleteTextures(1, &viewport_state.front_frame_tex);
            }

            if (viewport_state.back_frame_tex != 0) {
                glDeleteTextures(1, &viewport_state.back_frame_tex);
            }

            // back framebuffer texture
            glGenTextures(1, &viewport_state.back_frame_tex);
            glBindTexture(GL_TEXTURE_2D, viewport_state.back_frame_tex);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb_width, fb_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, viewport_state.back_fb);
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                    viewport_state.back_frame_tex, 0);

            glBindTexture(GL_TEXTURE_2D, 0);

            auto back_fb_status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
            if (back_fb_status != GL_FRAMEBUFFER_COMPLETE_EXT) {
                Logger::default_logger().fatal("Back framebuffer is incomplete (error %d)", back_fb_status);
            }

            // front framebuffer texture
            glGenTextures(1, &viewport_state.front_frame_tex);
            glBindTexture(GL_TEXTURE_2D, viewport_state.front_frame_tex);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb_width, fb_height, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, viewport_state.front_fb);
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                    viewport_state.front_frame_tex, 0);

            glBindTexture(GL_TEXTURE_2D, 0);

            auto front_fb_status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
            if (front_fb_status != GL_FRAMEBUFFER_COMPLETE_EXT) {
                Logger::default_logger().fatal("Front framebuffer is incomplete (error %d)", front_fb_status);
            }
        }

        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, viewport_state.front_fb);

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

            if (program_info.handle != last_program) {
                glUseProgram(program_info.handle);
                set_per_frame_global_uniforms(program_info);
                last_program = program_info.handle;

                auto view_mat = viewport_state.view_matrix;
                program_info.reflection.get_uniform_loc_and_then(SHADER_UBO_VIEWPORT, SHADER_UNIFORM_VIEWPORT_VM,
                        [view_mat](auto vm_loc) {
                            affirm_precond(vm_loc <= INT_MAX, "View matrix uniform location is too big");
                            glUniformMatrix4fv(GLint(vm_loc), 1, GL_FALSE, view_mat.data);
                        });
            }

            auto &stride = bucket.second->atlas_stride;
            program_info.reflection.get_uniform_loc_and_then(SHADER_UBO_OBJ, SHADER_UNIFORM_OBJ_UV_STRIDE,
                    [stride](auto loc) {
                        affirm_precond(loc <= INT_MAX, "UV stride uniform location is too big");
                        glUniform2f(GLint(loc), stride.x, stride.y);
                    });

            if (tex_handle != last_texture) {
                glBindTexture(GL_TEXTURE_2D, tex_handle);
                last_texture = tex_handle;
            }

            set_bucket_vbo_attribs(scene_state, *bucket.second);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glDrawArrays(GL_TRIANGLES, 0, GLsizei(bucket.second->vertex_count));
        }

        glBindTexture(GL_TEXTURE_2D, 0);

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

            glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, viewport_state.front_fb);

            glClearColor(0.0, 0.0, 0.0, 0.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(
                    0,
                    0,
                    fb_width,
                    fb_height
            );

            _set_fb_attribs(state.frame_vbo);

            glUseProgram(postfx_program->handle);
            set_per_frame_global_uniforms(*postfx_program);
            glBindTexture(GL_TEXTURE_2D, viewport_state.back_frame_tex);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
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

        _set_fb_attribs(state.frame_vbo);

        glUseProgram(state.frame_program.value().handle);
        glBindTexture(GL_TEXTURE_2D, viewport_state.front_frame_tex);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
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

        glGenBuffers(1, &state.frame_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, state.frame_vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(frame_quad_vertex_data), frame_quad_vertex_data, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}
