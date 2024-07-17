/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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
#include "argus/lowlevel/math.hpp"

#include "argus/core/engine.hpp"

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
#include "argus/render/2d/scene_2d.hpp"

#define BINDING_INDEX_VBO 0

namespace argus {
    struct TransformedViewport {
        int32_t top;
        int32_t bottom;
        int32_t left;
        int32_t right;
    };

    struct Std140Light2D {
        // offset 0
        float color[4];
        // offset 16
        float position[4];
        // offset 32
        float intensity;
        // offset 36
        uint32_t falloff_gradient;
        // offset 40
        float falloff_distance;
        // offset 44
        float falloff_buffer;
        // offset 48
        uint32_t shadow_falloff_gradient;
        // offset 52
        float shadow_falloff_distance;
        // offset 56
        int32_t type;
        // offset 60
        bool is_occludable;
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
                crash("Viewport mode is invalid");
        }

        TransformedViewport transformed {};
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

            bool must_update = false;

            if (!scene_state.ubo.valid) {
                scene_state.ubo = BufferInfo::create(GL_UNIFORM_BUFFER, SHADER_UBO_SCENE_LEN, GL_DYNAMIC_DRAW,
                        true, false);
                must_update = true;
            }

            if (must_update || al_level.dirty) {
                scene_state.ubo.write_val<float>(al_level.value, SHADER_UNIFORM_SCENE_AL_LEVEL_OFF);
            }

            if (must_update || al_color.dirty) {
                float color[4] = { al_color->r, al_color->g, al_color->b, 1.0 };
                scene_state.ubo.write(color, sizeof(color), SHADER_UNIFORM_SCENE_AL_COLOR_OFF);
            }

            Std140Light2D shader_lights_arr[LIGHTS_MAX];

            scene.lock_render_state();

            size_t i = 0;
            for (const auto &light : scene.get_lights_for_render()) {
                const auto &color = light.get().get_color();
                const auto pos = light.get().get_transform().get_translation();
                shader_lights_arr[i] = Std140Light2D {
                        { color.r, color.g, color.b, 1.0 }, // color
                        { pos.x, pos.y, 0.0, 1.0 }, // position
                        light.get().get_parameters().m_intensity,
                        light.get().get_parameters().m_falloff_gradient,
                        light.get().get_parameters().m_falloff_multiplier,
                        light.get().get_parameters().m_falloff_buffer,
                        light.get().get_parameters().m_shadow_falloff_gradient,
                        light.get().get_parameters().m_shadow_falloff_multiplier,
                        int(light.get().get_type()), // type
                        light.get().is_occludable(),
                };

                i++;
            }

            scene_state.ubo.write_val(scene.get_lights_for_render().size(), SHADER_UNIFORM_SCENE_LIGHT_COUNT_OFF);

            scene.unlock_render_state();

            scene_state.ubo.write(shader_lights_arr, sizeof(shader_lights_arr), SHADER_UNIFORM_SCENE_LIGHTS_OFF);
        }
    }

    static void _update_viewport_ubo(ViewportState &viewport_state) {
        bool must_update = viewport_state.view_matrix_dirty;

        if (!viewport_state.ubo.valid) {
            viewport_state.ubo = BufferInfo::create(GL_UNIFORM_BUFFER, SHADER_UBO_VIEWPORT_LEN, GL_STATIC_DRAW,
                    true, false);
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

        auto have_draw_buffers_blend = AGLET_GL_VERSION_4_0
                || AGLET_GL_ARB_draw_buffers_blend
                || AGLET_GL_AMD_draw_buffers_blend;

        // set scene uniforms
        _update_scene_ubo(scene_state);

        // set viewport uniforms
        _update_viewport_ubo(viewport_state);

        // shadowmap setup
        if (viewport_state.shadowmap_texture == 0) {
            viewport_state.shadowmap_buffer = BufferInfo::create(GL_TEXTURE_BUFFER, SHADER_IMAGE_SHADOWMAP_LEN,
                    GL_STREAM_COPY, false, false);
            if (AGLET_GL_ARB_direct_state_access) {
                glCreateTextures(GL_TEXTURE_BUFFER, 1, &viewport_state.shadowmap_texture);
                glTextureBuffer(viewport_state.shadowmap_texture, GL_R32UI, viewport_state.shadowmap_buffer.handle);
                glTextureParameteri(viewport_state.shadowmap_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTextureParameteri(viewport_state.shadowmap_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            } else {
                glGenTextures(1, &viewport_state.shadowmap_texture);
                glBindTextureUnit(GL_TEXTURE_BUFFER, viewport_state.shadowmap_texture);
                glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, viewport_state.shadowmap_buffer.handle);
                glTexParameteri(GL_TEXTURE_BUFFER, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_BUFFER, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }
        }

        // framebuffer setup
        if (viewport_state.fb_primary == 0) {
            if (AGLET_GL_ARB_direct_state_access) {
                glCreateFramebuffers(1, &viewport_state.fb_primary);
                glCreateFramebuffers(1, &viewport_state.fb_secondary);
                glCreateFramebuffers(1, &viewport_state.fb_aux);
                glCreateFramebuffers(1, &viewport_state.fb_lightmap);
            } else {
                glGenFramebuffers(1, &viewport_state.fb_primary);
                glGenFramebuffers(1, &viewport_state.fb_secondary);
                glGenFramebuffers(1, &viewport_state.fb_aux);
                glGenFramebuffers(1, &viewport_state.fb_lightmap);
            }
        }

        if (viewport_state.color_buf_primary == 0 || resolution.dirty) {
            if (viewport_state.color_buf_primary != 0) {
                glDeleteTextures(1, &viewport_state.color_buf_primary);
            }

            if (viewport_state.color_buf_secondary != 0) {
                glDeleteTextures(1, &viewport_state.color_buf_secondary);
            }

            if (viewport_state.light_opac_map_buf != 0) {
                glDeleteTextures(1, &viewport_state.light_opac_map_buf);
            }

            if (viewport_state.lightmap_buf != 0) {
                glDeleteTextures(1, &viewport_state.lightmap_buf);
            }

            if (AGLET_GL_ARB_direct_state_access) {
                // initialize primary color buffers
                glCreateTextures(GL_TEXTURE_2D, 1, &viewport_state.color_buf_primary);
                glCreateTextures(GL_TEXTURE_2D, 1, &viewport_state.color_buf_secondary);

                glTextureStorage2D(viewport_state.color_buf_primary, 1, GL_RGBA8, fb_width, fb_height);
                glTextureStorage2D(viewport_state.color_buf_secondary, 1, GL_RGBA8, fb_width, fb_height);

                glTextureParameteri(viewport_state.color_buf_primary, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTextureParameteri(viewport_state.color_buf_primary, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glTextureParameteri(viewport_state.color_buf_secondary, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTextureParameteri(viewport_state.color_buf_secondary, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                // initialize auxiliary buffers
                glCreateTextures(GL_TEXTURE_2D, 1, &viewport_state.light_opac_map_buf);
                glTextureStorage2D(viewport_state.light_opac_map_buf, 1, GL_R32F, fb_width, fb_height);
                glTextureParameteri(viewport_state.light_opac_map_buf, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTextureParameteri(viewport_state.light_opac_map_buf, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glCreateTextures(GL_TEXTURE_2D, 1, &viewport_state.lightmap_buf);
                glTextureStorage2D(viewport_state.lightmap_buf, 1, GL_RGBA8, fb_width, fb_height);
                glTextureParameteri(viewport_state.lightmap_buf, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTextureParameteri(viewport_state.lightmap_buf, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                // attach primary color buffers
                glNamedFramebufferTexture(viewport_state.fb_primary, GL_COLOR_ATTACHMENT0,
                        viewport_state.color_buf_primary, 0);
                glNamedFramebufferTexture(viewport_state.fb_secondary, GL_COLOR_ATTACHMENT0,
                        viewport_state.color_buf_secondary, 0);

                // attach auxiliary buffers
                glNamedFramebufferTexture(viewport_state.fb_primary, GL_COLOR_ATTACHMENT1,
                        viewport_state.light_opac_map_buf, 0);
                // don't attach aux buffers to the secondary fb so they don't get
                // lost while ping-ponging

                // need to be able to set a per-attachment blend
                // function + equation to be able to do it in one pass
                if (have_draw_buffers_blend) {
                    GLenum draw_bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
                    glNamedFramebufferDrawBuffers(viewport_state.fb_primary, 2, draw_bufs);
                }

                // set up second-pass auxiliary FBO
                glNamedFramebufferTexture(viewport_state.fb_aux, GL_COLOR_ATTACHMENT1,
                        viewport_state.light_opac_map_buf, 0);

                GLenum aux_draw_bufs[] = { GL_NONE, GL_COLOR_ATTACHMENT1 };
                glNamedFramebufferDrawBuffers(viewport_state.fb_aux, 2, aux_draw_bufs);

                // set up framebuffer for lighting pass
                glNamedFramebufferTexture(viewport_state.fb_lightmap, GL_COLOR_ATTACHMENT0,
                        viewport_state.lightmap_buf, 0);

                GLenum lightmap_draw_bufs[] = { GL_COLOR_ATTACHMENT0 };
                glNamedFramebufferDrawBuffers(viewport_state.fb_lightmap, 1, lightmap_draw_bufs);

                // check framebuffer statuses

                auto front_fb_status = glCheckNamedFramebufferStatus(viewport_state.fb_primary, GL_FRAMEBUFFER);
                if (front_fb_status != GL_FRAMEBUFFER_COMPLETE) {
                    crash("Front framebuffer is incomplete (error %d)", front_fb_status);
                }

                auto back_fb_status = glCheckNamedFramebufferStatus(viewport_state.fb_secondary, GL_FRAMEBUFFER);
                if (back_fb_status != GL_FRAMEBUFFER_COMPLETE) {
                    crash("Back framebuffer is incomplete (error %d)", back_fb_status);
                }

                auto aux_fb_status = glCheckNamedFramebufferStatus(viewport_state.fb_secondary, GL_FRAMEBUFFER);
                if (aux_fb_status != GL_FRAMEBUFFER_COMPLETE) {
                    crash("Opacity map framebuffer is incomplete (error %d)", aux_fb_status);
                }

                auto lm_fb_status = glCheckNamedFramebufferStatus(viewport_state.fb_secondary, GL_FRAMEBUFFER);
                if (lm_fb_status != GL_FRAMEBUFFER_COMPLETE) {
                    crash("Opacity map framebuffer is incomplete (error %d)", back_fb_status);
                }
            } else {
                // light opacity buffer
                glGenTextures(1, &viewport_state.light_opac_map_buf);
                bind_texture(0, viewport_state.light_opac_map_buf);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, fb_width, fb_height, 0,
                        GL_RED, GL_FLOAT, nullptr);

                // secondary framebuffer texture
                glGenTextures(1, &viewport_state.color_buf_secondary);
                bind_texture(0, viewport_state.color_buf_secondary);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb_width, fb_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                bind_texture(0, 0);

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.fb_secondary);

                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                        viewport_state.color_buf_secondary, 0);
                // don't attach aux buffers to the secondary fb so they don't get
                // lost while ping-ponging

                auto back_fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (back_fb_status != GL_FRAMEBUFFER_COMPLETE) {
                    crash("Back framebuffer is incomplete (error %d)", back_fb_status);
                }

                // primary framebuffer texture
                glGenTextures(1, &viewport_state.color_buf_primary);
                bind_texture(0, viewport_state.color_buf_primary);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb_width, fb_height, 0,
                        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                bind_texture(0, 0);

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.fb_primary);

                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                        viewport_state.color_buf_primary, 0);
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                        viewport_state.light_opac_map_buf, 0);

                // need to be able to set a per-attachment blend
                // function + equation to be able to do it in one pass
                if (have_draw_buffers_blend) {
                    GLenum draw_bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
                    glDrawBuffers(2, draw_bufs);
                }

                auto front_fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (front_fb_status != GL_FRAMEBUFFER_COMPLETE) {
                    crash("Front framebuffer is incomplete (error %d)", front_fb_status);
                }

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.fb_aux);

                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                        viewport_state.light_opac_map_buf, 0);

                GLenum draw_bufs[] = { GL_NONE, GL_COLOR_ATTACHMENT1 };
                glDrawBuffers(2, draw_bufs);

                auto aux_fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (aux_fb_status != GL_FRAMEBUFFER_COMPLETE) {
                    crash("Auxiliary framebuffer is incomplete (error %d)", front_fb_status);
                }
            }
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.fb_primary);

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

        std::vector<RenderBucket *> non_std_buckets;

        for (auto &[_, bucket] : scene_state.render_buckets) {
            auto &mat = bucket->material_res;
            auto &program_info = state.linked_programs.find(mat.prototype.uid)->second;
            auto &texture_uid = mat.get<Material>().get_texture_uid();
            auto tex_handle = state.prepared_textures.find(texture_uid)->second;

            if (!have_draw_buffers_blend || program_info.has_custom_frag) {
                non_std_buckets.push_back(bucket);
            }

            if (program_info.handle != last_program) {
                glUseProgram(program_info.handle);
                last_program = program_info.handle;

                _bind_ubo(program_info, SHADER_UBO_GLOBAL, state.global_ubo);
                _bind_ubo(program_info, SHADER_UBO_SCENE, scene_state.ubo);
                _bind_ubo(program_info, SHADER_UBO_VIEWPORT, viewport_state.ubo);
            }

            if (program_info.reflection.has_ubo(SHADER_UBO_OBJ)) {
                _bind_ubo(program_info, SHADER_UBO_OBJ, bucket->obj_ubo);
            }

            if (tex_handle != last_texture) {
                bind_texture(0, tex_handle);
                last_texture = tex_handle;
            }

            glBindVertexArray(bucket->vertex_array);

            //TODO: move this to material init
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glDrawArrays(GL_TRIANGLES, 0, GLsizei(bucket->vertex_count));

            glBindVertexArray(0);
        }

        if (!AGLET_GL_ARB_direct_state_access) {
            bind_texture(0, 0);
        }

        if (scene_state.scene.type == SceneType::TwoD) {
            if (reinterpret_cast<Scene2D *>(&scene_state.scene)->is_lighting_enabled()) {
                // generate shadowmap
                compute_scene_shadowmap(scene_state, viewport_state, resolution);

                // generate lightmap
                draw_scene_lightmap(scene_state, viewport_state, resolution);

                // draw lightmap to framebuffer
                //glUseProgram(get_lightmap_composite_program(state).handle);
                glUseProgram(state.frame_program->handle);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.fb_primary);

                glViewport(
                        0,
                        0,
                        fb_width,
                        fb_height
                );

                glBindVertexArray(state.frame_vao);
                //bind_texture(0, viewport_state.color_buf_primary);
                bind_texture(0, viewport_state.lightmap_buf);

                // blend color multiplicatively, don't touch destination alpha
                glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                restore_gl_blend_params();

                //std::swap(viewport_state.fb_primary, viewport_state.fb_secondary);
                //std::swap(viewport_state.color_buf_primary, viewport_state.color_buf_secondary);
            }
        }

        // set buffers for ping-ponging
        auto fb_front = viewport_state.fb_primary;
        auto fb_back = viewport_state.fb_secondary;
        auto color_buf_front = viewport_state.color_buf_primary;
        auto color_buf_back = viewport_state.color_buf_secondary;

        for (auto &postfx : viewport_state.viewport->get_postprocessing_shaders()) {
            LinkedProgram *postfx_program;

            auto &postfx_programs = state.postfx_programs;
            auto it = postfx_programs.find(postfx);
            if (it != postfx_programs.end()) {
                postfx_program = &it->second;
            } else {
                auto linked_program = link_program({ FB_SHADER_VERT_PATH, postfx });
                auto inserted = postfx_programs.emplace(postfx, linked_program);
                postfx_program = &inserted.first->second;
            }

            std::swap(fb_front, fb_back);
            std::swap(color_buf_front, color_buf_back);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb_front);

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
            bind_texture(0, color_buf_back);

            _bind_ubo(*postfx_program, SHADER_UBO_GLOBAL, state.global_ubo);
            _bind_ubo(*postfx_program, SHADER_UBO_SCENE, scene_state.ubo);
            _bind_ubo(*postfx_program, SHADER_UBO_VIEWPORT, viewport_state.ubo);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBindVertexArray(0);

        viewport_state.color_buf_front = color_buf_front;

        // do selective second pass to populate auxiliary buffers
        if (!non_std_buckets.empty()) {
            auto &std_program = get_std_program(state);
            std::string last_tex;

            if (!have_draw_buffers_blend) {
                glBlendEquation(GL_MAX);
            }

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.fb_aux);

            glClearColor(0.0, 0.0, 0.0, 0.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(
                    -viewport_px.left,
                    -viewport_px.top,
                    GLsizei(resolution->x),
                    GLsizei(resolution->y)
            );

            glUseProgram(std_program.handle);

            _bind_ubo(std_program, SHADER_UBO_GLOBAL, state.global_ubo);
            _bind_ubo(std_program, SHADER_UBO_SCENE, scene_state.ubo);
            _bind_ubo(std_program, SHADER_UBO_VIEWPORT, viewport_state.ubo);

            for (auto *bucket : non_std_buckets) {
                _bind_ubo(std_program, SHADER_UBO_OBJ, bucket->obj_ubo);

                auto &mat = bucket->material_res;
                auto &texture_uid = mat.get<Material>().get_texture_uid();

                if (texture_uid != last_tex) {
                    auto tex_handle = state.prepared_textures.find(texture_uid)->second;
                    bind_texture(0, tex_handle);
                    last_tex = texture_uid;
                }

                glBindVertexArray(bucket->vertex_array);

                //TODO: move this to material init
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glDrawArrays(GL_TRIANGLES, 0, GLsizei(bucket->vertex_count));
            }

            glBindVertexArray(0);

            if (!AGLET_GL_ARB_direct_state_access) {
                bind_texture(0, 0);
            }

            glUseProgram(0);

            if (!have_draw_buffers_blend) {
                glBlendEquation(GL_FUNC_ADD);
            }
        }

        bind_texture(0, 0);
        glUseProgram(0);
        glBindVertexArray(0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void compute_scene_shadowmap(SceneState &scene_state, ViewportState &viewport_state,
            ValueAndDirtyFlag<Vector2u> resolution) {
        UNUSED(resolution);

        auto &state = scene_state.parent_state;

        auto shadowmap_program = get_shadowmap_program(state);

        viewport_state.shadowmap_buffer.clear(UINT32_MAX);

        // the shadowmap shader discards fragments unconditionally
        // so we can just reuse the secondary framebuffer
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.fb_secondary);
        glBindVertexArray(state.frame_vao);
        glUseProgram(shadowmap_program.handle);

        _bind_ubo(shadowmap_program, SHADER_UBO_SCENE, scene_state.ubo);
        _bind_ubo(shadowmap_program, SHADER_UBO_VIEWPORT, viewport_state.ubo);

        bind_texture(0, viewport_state.light_opac_map_buf);

        glBindImageTexture(0, viewport_state.shadowmap_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);

        glUseProgram(0);
        glBindVertexArray(0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        //std::swap(viewport_state.fb_primary, viewport_state.fb_secondary);
        //std::swap(viewport_state.color_buf_primary, viewport_state.color_buf_secondary);
    }

    void draw_scene_lightmap(SceneState &scene_state, ViewportState &viewport_state,
            ValueAndDirtyFlag<Vector2u> resolution) {
        UNUSED(resolution);
        UNUSED(resolution);

        auto &state = scene_state.parent_state;

        auto lighting_program = get_lighting_program(state);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.fb_lightmap);
        glBindVertexArray(state.frame_vao);
        glUseProgram(lighting_program.handle);

        glClearColor(1, 1, 1, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _bind_ubo(lighting_program, SHADER_UBO_SCENE, scene_state.ubo);
        _bind_ubo(lighting_program, SHADER_UBO_VIEWPORT, viewport_state.ubo);

        bind_texture(0, viewport_state.shadowmap_texture);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);

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
        bind_texture(0, viewport_state.color_buf_front);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        bind_texture(0, 0);
        glUseProgram(0);
        glBindVertexArray(0);
    }

    void setup_framebuffer(RendererState &state) {
        auto frame_program = link_program({ FB_SHADER_VERT_PATH, FB_SHADER_FRAG_PATH });

        state.frame_program = frame_program;

        if (!frame_program.reflection.get_attr_loc(SHADER_ATTRIB_POSITION).has_value()) {
            crash("Frame program is missing required position attribute");
        }
        if (!frame_program.reflection.get_attr_loc(SHADER_ATTRIB_TEXCOORD).has_value()) {
            crash("Frame program is missing required texcoords attribute");
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
