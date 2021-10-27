/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

// module lowlevel
#include "argus/lowlevel/atomic.hpp"
#include "argus/lowlevel/math.hpp"
#include "internal/lowlevel/logging.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "internal/wm/pimpl/window.hpp"

// module resman
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

// module render
#include "argus/render/common/material.hpp"
#include "argus/render/common/renderer.hpp"
#include "internal/render/pimpl/common/material.hpp"

// module render_opengl
#include "internal/render_opengl/defines.hpp"
#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/renderer/compositing.hpp"
#include "internal/render_opengl/renderer/shader_mgmt.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"
#include "internal/render_opengl/state/scene_state.hpp"

#include "aglet/aglet.h"

#include <atomic>
#include <map>
#include <string>
#include <utility>

namespace argus {
    void draw_scene_to_framebuffer(SceneState &scene_state) {
        auto &state = scene_state.parent_state;
        auto &renderer = state.renderer;

        // framebuffer setup
        if (scene_state.framebuffer == 0) {
            if (AGLET_GL_ARB_direct_state_access) {
                glCreateFramebuffers(1, &scene_state.framebuffer);
            } else {
                glGenFramebuffers(1, &scene_state.framebuffer);
            }
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scene_state.framebuffer);

        if (scene_state.frame_texture == 0 || renderer.get_window().pimpl->dirty_resolution) {
            auto window_res = renderer.get_window().get_resolution();

            if (scene_state.frame_texture != 0) {
                glDeleteTextures(1, &scene_state.frame_texture);
            }

            if (AGLET_GL_ARB_direct_state_access) {
                glCreateTextures(GL_TEXTURE_2D, 1, &scene_state.frame_texture);

                glTextureStorage2D(scene_state.frame_texture, 1, GL_RGBA8, window_res.x, window_res.y);

                glTextureParameteri(scene_state.frame_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTextureParameteri(scene_state.frame_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glNamedFramebufferTexture(scene_state.framebuffer, GL_COLOR_ATTACHMENT0, scene_state.frame_texture, 0);
            } else {
                glGenTextures(1, &scene_state.frame_texture);
                glBindTexture(GL_TEXTURE_2D, scene_state.frame_texture);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_res.x, window_res.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, scene_state.frame_texture, 0);

                glBindTexture(GL_TEXTURE_2D, 0);
            }

            auto fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
                _ARGUS_FATAL("Framebuffer is incomplete (error %d)\n", fb_status);
            }
        }

        // clear framebuffer
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Vector2u window_res = renderer.get_window().pimpl->properties.resolution;

        glViewport(0, 0, window_res.x, window_res.y);

        program_handle_t last_program = 0;
        texture_handle_t last_texture = 0;

        for (auto &bucket : scene_state.render_buckets) {
            auto &mat = bucket.second->material_res;
            auto &program_info = state.linked_programs.find(mat.uid)->second;
            auto &texture_uid = mat.get<Material>().pimpl->texture;
            auto tex_handle = state.prepared_textures.find(texture_uid)->second;

            if (program_info.handle != last_program) {
                glUseProgram(program_info.handle);
                last_program = program_info.handle;

                auto view_mat_loc = program_info.view_matrix_uniform_loc;
                if (view_mat_loc != -1) {
                    glUniformMatrix4fv(view_mat_loc, 1, GL_TRUE, scene_state.view_matrix.data);
                }
            }

            if (tex_handle != last_texture) {
                glBindTexture(GL_TEXTURE_2D, tex_handle);
                last_texture = tex_handle;
            }

            glBindVertexArray(bucket.second->vertex_array);

            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(bucket.second->vertex_count));

            glBindVertexArray(0);
        }

        if (!AGLET_GL_ARB_direct_state_access) {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glUseProgram(0);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void draw_framebuffer_to_screen(SceneState &scene_state) {
        auto &state = scene_state.parent_state;
        auto &renderer = state.renderer;

        Vector2u window_res = renderer.get_window().pimpl->properties.resolution;

        glViewport(0, 0, window_res.x, window_res.y);

        glBindVertexArray(state.frame_vao);
        glUseProgram(state.frame_program);
        glBindTexture(GL_TEXTURE_2D, scene_state.frame_texture);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        glBindVertexArray(0);
    }

    void setup_framebuffer(RendererState &state) {
        auto &fb_vert_shader_res = ResourceManager::instance().get_resource(FB_SHADER_VERT_PATH);
        auto &fb_frag_shader_res = ResourceManager::instance().get_resource(FB_SHADER_FRAG_PATH);

        state.frame_vert_shader = compile_shader(fb_vert_shader_res);
        state.frame_frag_shader = compile_shader(fb_frag_shader_res);

        state.frame_program = glCreateProgram();

        glAttachShader(state.frame_program, state.frame_vert_shader);
        glAttachShader(state.frame_program, state.frame_frag_shader);

        link_program(state.frame_program, VertexAttributes::POSITION | VertexAttributes::TEXCOORD);

        float frame_quad_vertex_data[] = {
            -1.0, -1.0,  0.0,  0.0,
            -1.0,  1.0,  0.0,  1.0,
             1.0,  1.0,  1.0,  1.0,
            -1.0, -1.0,  0.0,  0.0,
             1.0,  1.0,  1.0,  1.0,
             1.0, -1.0,  1.0,  0.0,
        };

        if (AGLET_GL_ARB_direct_state_access) {
            glCreateVertexArrays(1, &state.frame_vao);

            glCreateBuffers(1, &state.frame_vbo);
        
            glNamedBufferData(state.frame_vbo, sizeof(frame_quad_vertex_data), frame_quad_vertex_data, GL_STATIC_DRAW);
        } else {
            glGenVertexArrays(1, &state.frame_vao);
            glBindVertexArray(state.frame_vao);

            glGenBuffers(1, &state.frame_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, state.frame_vbo);

            glBufferData(GL_ARRAY_BUFFER, sizeof(frame_quad_vertex_data), frame_quad_vertex_data, GL_STATIC_DRAW);
        }

        unsigned int attr_offset = 0;
        set_attrib_pointer(state.frame_vao, state.frame_vbo, 4, SHADER_ATTRIB_IN_POSITION_LEN, SHADER_ATTRIB_LOC_POSITION, &attr_offset);
        set_attrib_pointer(state.frame_vao, state.frame_vbo, 4, SHADER_ATTRIB_IN_TEXCOORD_LEN, SHADER_ATTRIB_LOC_TEXCOORD, &attr_offset);

        if (!AGLET_GL_ARB_direct_state_access) {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    }
}
