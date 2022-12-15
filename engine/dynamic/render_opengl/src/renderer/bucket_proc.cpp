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

#include "argus/lowlevel/debug.hpp"

#include "argus/resman/resource.hpp"

#include "argus/render/common/material.hpp"
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
#include <utility>
#include <vector>

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
            _ARGUS_ASSERT(program_it != scene_state.parent_state.linked_programs.cend(),
                    "Cannot find material program");
            bool animated = program_it->second.has_uniform(SHADER_UNIFORM_UV_STRIDE);

            // the program should have been linked during object processing
            auto &program = scene_state.parent_state.linked_programs.find(bucket->material_res.uid)->second;

            auto attr_position_loc = program.get_attr_loc(SHADER_ATTRIB_POSITION);
            auto attr_normal_loc = program.get_attr_loc(SHADER_ATTRIB_NORMAL);
            auto attr_color_loc = program.get_attr_loc(SHADER_ATTRIB_COLOR);
            auto attr_texcoord_loc = program.get_attr_loc(SHADER_ATTRIB_TEXCOORD);
            auto attr_anim_frame_loc = program.get_attr_loc(SHADER_ATTRIB_ANIM_FRAME);

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
                Logger::default_logger().debug("anim_frame_buf_len: %d", anim_frame_buf_len);

                if (bucket->vertex_array != 0) {
                    glDeleteVertexArrays(1, &bucket->vertex_array);
                }

                if (bucket->vertex_buffer != 0) {
                    glDeleteBuffers(1, &bucket->vertex_buffer);
                }

                if (bucket->anim_frame_buffer != 0) {
                    glDeleteBuffers(1, &bucket->anim_frame_buffer);
                }

                if (AGLET_GL_ARB_direct_state_access) {
                    glCreateVertexArrays(1, &bucket->vertex_array);

                    glCreateBuffers(1, &bucket->vertex_buffer);
                    glNamedBufferData(bucket->vertex_buffer, buffer_len, nullptr, GL_DYNAMIC_COPY);

                    glVertexArrayVertexBuffer(bucket->vertex_array, BINDING_INDEX_VBO, bucket->vertex_buffer, 0,
                            vertex_len * static_cast<uint32_t>(sizeof(GLfloat)));

                    if (animated) {
                        glCreateBuffers(1, &bucket->anim_frame_buffer);
                        glNamedBufferData(bucket->anim_frame_buffer, anim_frame_buf_len, nullptr, GL_DYNAMIC_COPY);

                        glVertexArrayVertexBuffer(bucket->vertex_array, BINDING_INDEX_ANIM_FRAME_BUF,
                                bucket->anim_frame_buffer, 0,
                                SHADER_ATTRIB_ANIM_FRAME_LEN * static_cast<uint32_t>(sizeof(GLfloat)));
                    }
                } else {
                    glGenVertexArrays(1, &bucket->vertex_array);
                    glBindVertexArray(bucket->vertex_array);

                    if (animated) {
                        glGenBuffers(1, &bucket->anim_frame_buffer);
                        glBindBuffer(GL_ARRAY_BUFFER, bucket->anim_frame_buffer);
                        glBufferData(GL_ARRAY_BUFFER, anim_frame_buf_len, nullptr, GL_DYNAMIC_COPY);
                    }

                    glGenBuffers(1, &bucket->vertex_buffer);
                    glBindBuffer(GL_ARRAY_BUFFER, bucket->vertex_buffer);
                    glBufferData(GL_ARRAY_BUFFER, buffer_len, nullptr, GL_DYNAMIC_COPY);
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
            }

            bucket->vertex_count = 0;

            void *anim_frame_buf;
            if (animated) {
                if (AGLET_GL_ARB_direct_state_access) {
                    anim_frame_buf = glMapNamedBuffer(bucket->anim_frame_buffer, GL_WRITE_ONLY);
                } else {
                    glBindBuffer(GL_ARRAY_BUFFER, bucket->anim_frame_buffer);
                    anim_frame_buf = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
                }
            }

            if (!AGLET_GL_ARB_direct_state_access) {
                glBindBuffer(GL_ARRAY_BUFFER, bucket->vertex_buffer);
            }

            size_t offset = 0;
            size_t anim_frame_off = 0;
            for (auto *processed : bucket->objects) {
                if (processed == nullptr) {
                    continue;
                }

                if (bucket->needs_rebuild || processed->updated) {
                    if (AGLET_GL_ARB_direct_state_access) {
                        glCopyNamedBufferSubData(processed->staging_buffer, bucket->vertex_buffer, 0, offset,
                                processed->staging_buffer_size);
                    } else {
                        glBindBuffer(GL_COPY_READ_BUFFER, processed->staging_buffer);
                        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, offset,
                                processed->staging_buffer_size);
                        glBindBuffer(GL_COPY_READ_BUFFER, 0);
                    }
                }

                if (animated && (bucket->needs_rebuild || processed->anim_frame_updated)) {
                    for (size_t i = 0; i < processed->vertex_count; i++) {
                        reinterpret_cast<GLfloat*>(anim_frame_buf)[anim_frame_off++]
                                = static_cast<float>(processed->anim_frame.x);
                        reinterpret_cast<GLfloat*>(anim_frame_buf)[anim_frame_off++]
                                = static_cast<float>(processed->anim_frame.y);
                    }
                    processed->anim_frame_updated = false;
                } else {
                    anim_frame_off += processed->vertex_count * SHADER_ATTRIB_ANIM_FRAME_LEN;
                }

                offset += processed->staging_buffer_size;

                bucket->vertex_count += processed->vertex_count;
            }

            if (AGLET_GL_ARB_direct_state_access) {
                if (animated) {
                    glUnmapNamedBuffer(bucket->anim_frame_buffer);
                }
            } else {
                if (animated) {
                    glBindBuffer(GL_ARRAY_BUFFER, bucket->anim_frame_buffer);
                    glUnmapBuffer(bucket->anim_frame_buffer);
                }

                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glBindVertexArray(0);
            }

            bucket->needs_rebuild = false;

            it++;
        }
    }
}
