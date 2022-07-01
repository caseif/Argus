/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/math.hpp"

#include "argus/wm/window.hpp"
#include "internal/wm/pimpl/window.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/canvas.hpp"
#include "argus/render/common/material.hpp"

#include "internal/render_opengles/defines.hpp"
#include "internal/render_opengles/types.hpp"
#include "internal/render_opengles/gl_util.hpp"
#include "internal/render_opengles/renderer/compositing.hpp"
#include "internal/render_opengles/renderer/shader_mgmt.hpp"
#include "internal/render_opengles/state/render_bucket.hpp"
#include "internal/render_opengles/state/renderer_state.hpp"
#include "internal/render_opengles/state/scene_state.hpp"

#include "aglet/aglet.h"

#include <atomic>
#include <map>
#include <string>
#include <utility>

namespace argus {
    void draw_scene_to_framebuffer(SceneState &scene_state, ValueAndDirtyFlag<Vector2u> resolution) {
        auto &state = scene_state.parent_state;

        // framebuffer setup
        if (scene_state.framebuffer == 0) {
            glGenFramebuffers(1, &scene_state.framebuffer);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scene_state.framebuffer);

        if (scene_state.frame_texture == 0 || resolution.dirty) {
            if (scene_state.frame_texture != 0) {
                glDeleteTextures(1, &scene_state.frame_texture);
            }

            glGenTextures(1, &scene_state.frame_texture);
            glBindTexture(GL_TEXTURE_2D, scene_state.frame_texture);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resolution->x, resolution->y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene_state.frame_texture, 0);

            glBindTexture(GL_TEXTURE_2D, 0);

            auto fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
                Logger::default_logger().fatal("Framebuffer is incomplete (error %d)", fb_status);
            }
        }

        // clear framebuffer
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, resolution->x, resolution->y);

        program_handle_t last_program = 0;
        texture_handle_t last_texture = 0;

        for (auto &bucket : scene_state.render_buckets) {
            auto &mat = bucket.second->material_res;
            auto &program_info = state.linked_programs.find(mat.uid)->second;
            auto &texture_uid = mat.get<Material>().get_texture_uid();
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

        glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(0);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void draw_framebuffer_to_screen(SceneState &scene_state, ValueAndDirtyFlag<Vector2u> resolution) {
        auto &state = scene_state.parent_state;

        if (resolution.dirty) {
            glViewport(0, 0, resolution->x, resolution->y);
        }

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

        link_program(state.frame_program);
        auto frame_program = link_program(state.frame_program);

        if (!frame_program.attr_position_loc.has_value() || !frame_program.attr_texcoord_loc.has_value()) {
            Logger::default_logger().fatal("Frame program is missing required attributes");
        }

        float frame_quad_vertex_data[] = {
            -1.0, -1.0,  0.0,  0.0,
            -1.0,  1.0,  0.0,  1.0,
             1.0,  1.0,  1.0,  1.0,
            -1.0, -1.0,  0.0,  0.0,
             1.0,  1.0,  1.0,  1.0,
             1.0, -1.0,  1.0,  0.0,
        };

        glGenVertexArrays(1, &state.frame_vao);
        glBindVertexArray(state.frame_vao);

        glGenBuffers(1, &state.frame_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, state.frame_vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(frame_quad_vertex_data), frame_quad_vertex_data, GL_STATIC_DRAW);

        unsigned int attr_offset = 0;
        set_attrib_pointer(4, SHADER_ATTRIB_IN_POSITION_LEN, frame_program.attr_position_loc.value(), &attr_offset);
        set_attrib_pointer(4, SHADER_ATTRIB_IN_TEXCOORD_LEN, frame_program.attr_texcoord_loc.value(), &attr_offset);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}
