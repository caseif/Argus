/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module wm
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
#include "internal/render_opengl/state/layer_state.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"

#include "aglet/aglet.h"

namespace argus {
    void draw_layer_to_framebuffer(LayerState &layer_state) {
        auto &state = layer_state.parent_state;
        auto &renderer = state.renderer;

        // framebuffer setup
        if (layer_state.framebuffer == 0) {
            glGenFramebuffers(1, &layer_state.framebuffer);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, layer_state.framebuffer);

        if (layer_state.frame_texture == 0 || renderer.get_window().pimpl->dirty_resolution) {
             if (layer_state.frame_texture != 0) {
                 glDeleteTextures(1, &layer_state.frame_texture);
             }

             glGenTextures(1, &layer_state.frame_texture);
             glBindTexture(GL_TEXTURE_2D, layer_state.frame_texture);

             auto res = renderer.get_window().get_resolution();
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res.x, res.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

             glBindTexture(GL_TEXTURE_2D, 0);

             glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, layer_state.frame_texture, 0);

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

        for (auto &bucket : layer_state.render_buckets) {
            auto &mat = bucket.second->material_res;
            auto &program_info = state.linked_programs.find(mat.uid)->second;
            auto &texture_uid = mat.get<Material>().pimpl->texture;
            auto tex_handle = state.prepared_textures.find(texture_uid)->second;

            if (program_info.handle != last_program) {
                glUseProgram(program_info.handle);
                last_program = program_info.handle;

                auto view_mat_loc = program_info.view_matrix_uniform_loc;
                if (view_mat_loc != -1) {
                    glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, layer_state.view_matrix);
                }
            }

            if (tex_handle != last_texture) {
                glBindTexture(GL_TEXTURE_2D, tex_handle);
                last_texture = tex_handle;
            }

            glBindVertexArray(bucket.second->vertex_array);

            glDrawArrays(GL_TRIANGLES, 0, bucket.second->vertex_count);

            glBindVertexArray(0);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void draw_framebuffer_to_screen(LayerState &layer_state) {
        auto &state = layer_state.parent_state;
        auto &renderer = state.renderer;

        Vector2u window_res = renderer.get_window().pimpl->properties.resolution;

        glViewport(0, 0, window_res.x, window_res.y);

        glBindVertexArray(state.frame_vao);

        glUseProgram(state.frame_program);

        glBindTexture(GL_TEXTURE_2D, layer_state.frame_texture);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        glBindVertexArray(0);
    }

    void setup_framebuffer(RendererState &state) {
        auto &fb_vert_shader_res = ResourceManager::get_global_resource_manager().get_resource(FB_SHADER_VERT_PATH);
        auto &fb_frag_shader_res = ResourceManager::get_global_resource_manager().get_resource(FB_SHADER_FRAG_PATH);

        state.frame_vert_shader = compile_shader(fb_vert_shader_res);
        state.frame_frag_shader = compile_shader(fb_frag_shader_res);

        state.frame_program = glCreateProgram();

        glAttachShader(state.frame_program, state.frame_vert_shader);
        glAttachShader(state.frame_program, state.frame_frag_shader);

        link_program(state.frame_program, VertexAttributes::POSITION | VertexAttributes::TEXCOORD);

        glGenVertexArrays(1, &state.frame_vao);
        glBindVertexArray(state.frame_vao);

        glGenBuffers(1, &state.frame_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, state.frame_vbo);

        float frame_quad_vertex_data[] = {
            -1.0, -1.0,  0.0,  0.0,
            -1.0,  1.0,  0.0,  1.0,
             1.0,  1.0,  1.0,  1.0,
            -1.0, -1.0,  0.0,  0.0,
             1.0,  1.0,  1.0,  1.0,
             1.0, -1.0,  1.0,  0.0,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(frame_quad_vertex_data), frame_quad_vertex_data, GL_STATIC_DRAW);

        unsigned int attr_offset = 0;
        set_attrib_pointer(4, SHADER_ATTRIB_IN_POSITION_LEN, SHADER_ATTRIB_LOC_POSITION, &attr_offset);
        set_attrib_pointer(4, SHADER_ATTRIB_IN_TEXCOORD_LEN, SHADER_ATTRIB_LOC_TEXCOORD, &attr_offset);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}
