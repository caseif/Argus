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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/types.hpp"

#include "argus/render/defines.hpp"

#include "internal/render_opengl/defines.hpp"
#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/renderer/bucket_proc.hpp"
#include "internal/render_opengl/state/processed_render_object.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"
#include "internal/render_opengl/state/scene_state.hpp"

#include "aglet/aglet.h"

#include <map>
#include <string>

#include <climits>
#include <cstddef>

#define BINDING_INDEX_VBO 0
#define BINDING_INDEX_ANIM_FRAME_BUF 1

namespace argus {
    void fill_buckets(SceneState &scene_state) {
        for (auto it = scene_state.render_buckets.begin(); it != scene_state.render_buckets.end();) {
            auto *bucket = it->second;

            if (bucket->objects.empty()) {
                try_delete_buffer(it->second->vertex_array);
                try_delete_buffer(it->second->vertex_buffer);
                try_delete_buffer(it->second->anim_frame_buffer);
                it->second->~RenderBucket();

                it = scene_state.render_buckets.erase(it);

                continue;
            }

            auto program_it = scene_state.parent_state.linked_programs.find(bucket->material_res.uid);
            affirm_precond(program_it != scene_state.parent_state.linked_programs.cend(),
                    "Cannot find material program");
            bool animated = program_it->second.reflection.has_uniform(SHADER_UNIFORM_UV_STRIDE);

            // the program should have been linked during object processing
            auto &program = scene_state.parent_state.linked_programs.find(bucket->material_res.uid)->second;

            auto attr_position_loc = program.reflection.get_attr_loc(SHADER_ATTRIB_POSITION);
            auto attr_normal_loc = program.reflection.get_attr_loc(SHADER_ATTRIB_NORMAL);
            auto attr_color_loc = program.reflection.get_attr_loc(SHADER_ATTRIB_COLOR);
            auto attr_texcoord_loc = program.reflection.get_attr_loc(SHADER_ATTRIB_TEXCOORD);
            auto attr_anim_frame_loc = program.reflection.get_attr_loc(SHADER_ATTRIB_ANIM_FRAME);

            uint32_t vertex_len = (attr_position_loc.has_value() ? SHADER_ATTRIB_POSITION_LEN : 0)
                                  + (attr_normal_loc.has_value() ? SHADER_ATTRIB_NORMAL_LEN : 0)
                                  + (attr_color_loc.has_value() ? SHADER_ATTRIB_COLOR_LEN : 0)
                                  + (attr_texcoord_loc.has_value() ? SHADER_ATTRIB_TEXCOORD_LEN : 0);

            size_t anim_frame_buf_len = 0;
            if (bucket->needs_rebuild) {
                size_t buffer_len = 0;
                for (auto &obj : bucket->objects) {
                    buffer_len += obj->staging_buffer_size;
                    anim_frame_buf_len += obj->vertex_count * SHADER_ATTRIB_ANIM_FRAME_LEN * sizeof(GLfloat);
                }

                if (bucket->vertex_array != 0) {
                    glDeleteVertexArrays(1, &bucket->vertex_array);
                }

                if (bucket->vertex_buffer != 0) {
                    glDeleteBuffers(1, &bucket->vertex_buffer);
                }

                if (bucket->anim_frame_buffer != 0) {
                    glDeleteBuffers(1, &bucket->anim_frame_buffer);
                }

                affirm_precond(buffer_len <= INT_MAX, "Buffer length is too big");

                if (AGLET_GL_ARB_direct_state_access) {
                    glCreateVertexArrays(1, &bucket->vertex_array);

                    glCreateBuffers(1, &bucket->vertex_buffer);
                    glNamedBufferData(bucket->vertex_buffer, GLsizei(buffer_len), nullptr, GL_DYNAMIC_COPY);

                    auto stride = vertex_len * uint(sizeof(GLfloat));
                    affirm_precond(stride <= INT_MAX, "Vertex stride is too big");

                    glVertexArrayVertexBuffer(bucket->vertex_array, BINDING_INDEX_VBO, bucket->vertex_buffer, 0,
                            GLsizei(stride));

                    if (animated) {
                        glCreateBuffers(1, &bucket->anim_frame_buffer);
                        affirm_precond(anim_frame_buf_len <= INT_MAX, "Animation frame buffer length is too big");
                        glNamedBufferData(bucket->anim_frame_buffer, GLsizei(anim_frame_buf_len), nullptr,
                                GL_DYNAMIC_DRAW);

                        glVertexArrayVertexBuffer(bucket->vertex_array, BINDING_INDEX_ANIM_FRAME_BUF,
                                bucket->anim_frame_buffer, 0,
                                SHADER_ATTRIB_ANIM_FRAME_LEN * GLsizei(sizeof(GLfloat)));
                    }
                } else {
                    glGenVertexArrays(1, &bucket->vertex_array);
                    glBindVertexArray(bucket->vertex_array);

                    if (animated) {
                        glGenBuffers(1, &bucket->anim_frame_buffer);
                        glBindBuffer(GL_ARRAY_BUFFER, bucket->anim_frame_buffer);
                        affirm_precond(anim_frame_buf_len <= INT_MAX, "Animation frame buffer length is too big");
                        glBufferData(GL_ARRAY_BUFFER, GLsizei(anim_frame_buf_len), nullptr, GL_DYNAMIC_DRAW);
                    }

                    glGenBuffers(1, &bucket->vertex_buffer);
                    glBindBuffer(GL_ARRAY_BUFFER, bucket->vertex_buffer);
                    glBufferData(GL_ARRAY_BUFFER, GLsizei(buffer_len), nullptr, GL_DYNAMIC_COPY);
                }

                if (animated) {
                    if (bucket->anim_frame_buffer_staging != nullptr) {
                        free(bucket->anim_frame_buffer_staging);
                    }

                    if (anim_frame_buf_len > 0) {
                        bucket->anim_frame_buffer_staging = std::calloc(1, anim_frame_buf_len);
                    } else {
                        bucket->anim_frame_buffer_staging = nullptr;
                    }
                }

                GLuint attr_offset = 0;

                if (attr_position_loc.has_value()) {
                    set_attrib_pointer(bucket->vertex_array, bucket->vertex_buffer, BINDING_INDEX_VBO, vertex_len,
                            SHADER_ATTRIB_POSITION_LEN, attr_position_loc.value(), &attr_offset);
                }
                if (attr_normal_loc.has_value()) {
                    set_attrib_pointer(bucket->vertex_array, bucket->vertex_buffer, BINDING_INDEX_VBO, vertex_len,
                            SHADER_ATTRIB_NORMAL_LEN, attr_normal_loc.value(), &attr_offset);
                }
                if (attr_color_loc.has_value()) {
                    set_attrib_pointer(bucket->vertex_array, bucket->vertex_buffer, BINDING_INDEX_VBO, vertex_len,
                            SHADER_ATTRIB_COLOR_LEN, attr_color_loc.value(), &attr_offset);
                }
                if (attr_texcoord_loc.has_value()) {
                    set_attrib_pointer(bucket->vertex_array, bucket->vertex_buffer, BINDING_INDEX_VBO, vertex_len,
                            SHADER_ATTRIB_TEXCOORD_LEN, attr_texcoord_loc.value(), &attr_offset);
                }
                if (attr_anim_frame_loc.has_value()) {
                    GLuint offset = 0;
                    set_attrib_pointer(bucket->vertex_array, bucket->anim_frame_buffer, BINDING_INDEX_ANIM_FRAME_BUF,
                            SHADER_ATTRIB_ANIM_FRAME_LEN, SHADER_ATTRIB_ANIM_FRAME_LEN, attr_anim_frame_loc.value(),
                            &offset);
                }
            } else {
                anim_frame_buf_len = bucket->vertex_count * SHADER_ATTRIB_ANIM_FRAME_LEN * sizeof(GLfloat);
            }

            bucket->vertex_count = 0;

            if (!AGLET_GL_ARB_direct_state_access) {
                glBindBuffer(GL_ARRAY_BUFFER, bucket->vertex_buffer);
            }

            bool anim_buf_updated = false;

            size_t offset = 0;
            size_t anim_frame_off = 0;
            for (auto *processed : bucket->objects) {
                if (processed == nullptr) {
                    continue;
                }

                if (bucket->needs_rebuild || processed->updated) {
                    affirm_precond(offset <= INT_MAX, "Buffer offset is too big");
                    affirm_precond(processed->staging_buffer_size <= INT_MAX, "Buffer offset is too big");

                    if (AGLET_GL_ARB_direct_state_access) {
                        glCopyNamedBufferSubData(processed->staging_buffer, bucket->vertex_buffer, 0, GLintptr(offset),
                                GLsizeiptr(processed->staging_buffer_size));
                    } else {
                        glBindBuffer(GL_COPY_READ_BUFFER, processed->staging_buffer);
                        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, GLintptr(offset),
                                GLsizeiptr(processed->staging_buffer_size));
                        glBindBuffer(GL_COPY_READ_BUFFER, 0);
                    }
                }

                if (animated && (bucket->needs_rebuild || processed->anim_frame_updated)) {
                    for (size_t i = 0; i < processed->vertex_count; i++) {
                        reinterpret_cast<GLfloat *>(bucket->anim_frame_buffer_staging)[anim_frame_off++]
                                = float(processed->anim_frame.x);
                        reinterpret_cast<GLfloat *>(bucket->anim_frame_buffer_staging)[anim_frame_off++]
                                = float(processed->anim_frame.y);
                    }
                    processed->anim_frame_updated = false;
                    anim_buf_updated = true;
                } else {
                    anim_frame_off += processed->vertex_count * SHADER_ATTRIB_ANIM_FRAME_LEN;
                }

                offset += processed->staging_buffer_size;

                bucket->vertex_count += processed->vertex_count;
            }

            if (anim_buf_updated) {
                affirm_precond(anim_frame_buf_len <= INT_MAX, "Animated frame buffer length is too big");

                if (AGLET_GL_ARB_direct_state_access) {
                    glNamedBufferSubData(bucket->anim_frame_buffer, 0, GLsizeiptr(anim_frame_buf_len),
                            bucket->anim_frame_buffer_staging);
                } else {
                    glBindBuffer(GL_ARRAY_BUFFER, bucket->anim_frame_buffer);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, GLsizeiptr(anim_frame_buf_len),
                            bucket->anim_frame_buffer_staging);
                }
            }

            if (!AGLET_GL_ARB_direct_state_access) {
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glBindVertexArray(0);
            }

            bucket->needs_rebuild = false;

            it++;
        }
    }
}
