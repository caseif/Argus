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

namespace argus {
    void fill_buckets(SceneState &scene_state) {
        for (auto it = scene_state.render_buckets.begin(); it != scene_state.render_buckets.end();) {
            auto *bucket = it->second;

            if (bucket->objects.empty()) {
                try_delete_buffer(it->second->vertex_array);
                try_delete_buffer(it->second->vertex_buffer);
                it->second->~RenderBucket();

                it = scene_state.render_buckets.erase(it);

                continue;
            }

            if (bucket->needs_rebuild) {
                size_t buffer_len = 0;
                for (auto &obj : bucket->objects) {
                    buffer_len += obj->staging_buffer_size;
                }

                if (bucket->vertex_array != 0) {
                    glDeleteVertexArrays(1, &bucket->vertex_array);
                }

                if (bucket->vertex_buffer != 0) {
                    glDeleteBuffers(1, &bucket->vertex_buffer);
                }

                if (AGLET_GL_ARB_direct_state_access) {
                    glCreateVertexArrays(1, &bucket->vertex_array);

                    glCreateBuffers(1, &bucket->vertex_buffer);

                    glNamedBufferData(bucket->vertex_buffer, buffer_len, nullptr, GL_DYNAMIC_COPY);
                } else {
                    glGenVertexArrays(1, &bucket->vertex_array);
                    glBindVertexArray(bucket->vertex_array);

                    glGenBuffers(1, &bucket->vertex_buffer);
                    glBindBuffer(GL_ARRAY_BUFFER, bucket->vertex_buffer);

                    glBufferData(GL_ARRAY_BUFFER, buffer_len, nullptr, GL_DYNAMIC_COPY);
                }

                // the program should have been linked during object processing
                auto &program = scene_state.parent_state.linked_programs.find(bucket->material_res.uid)->second;

                auto attr_position_loc = program.get_attr_loc(SHADER_ATTRIB_POSITION);
                auto attr_normal_loc = program.get_attr_loc(SHADER_ATTRIB_NORMAL);
                auto attr_color_loc = program.get_attr_loc(SHADER_ATTRIB_COLOR);
                auto attr_texcoord_loc = program.get_attr_loc(SHADER_ATTRIB_TEXCOORD);

                uint32_t vertex_len = (attr_position_loc.has_value() ? SHADER_ATTRIB_POSITION_LEN : 0)
                        + (attr_normal_loc.has_value() ? SHADER_ATTRIB_NORMAL_LEN : 0)
                        + (attr_color_loc.has_value() ? SHADER_ATTRIB_COLOR_LEN : 0)
                        + (attr_texcoord_loc.has_value() ? SHADER_ATTRIB_TEXCOORD_LEN : 0);

                GLuint attr_offset = 0;

                if (attr_position_loc.has_value()) {
                    set_attrib_pointer(bucket->vertex_array, bucket->vertex_buffer, vertex_len,
                            SHADER_ATTRIB_POSITION_LEN, attr_position_loc.value(), &attr_offset);
                }
                if (attr_normal_loc.has_value()) {
                    set_attrib_pointer(bucket->vertex_array, bucket->vertex_buffer, vertex_len,
                            SHADER_ATTRIB_NORMAL_LEN, attr_normal_loc.value(), &attr_offset);
                }
                if (attr_color_loc.has_value()) {
                    set_attrib_pointer(bucket->vertex_array, bucket->vertex_buffer, vertex_len,
                            SHADER_ATTRIB_COLOR_LEN, attr_color_loc.value(), &attr_offset);
                }
                if (attr_texcoord_loc.has_value()) {
                    set_attrib_pointer(bucket->vertex_array, bucket->vertex_buffer, vertex_len,
                            SHADER_ATTRIB_TEXCOORD_LEN, attr_texcoord_loc.value(), &attr_offset);
                }
            } else {
                if (!AGLET_GL_ARB_direct_state_access) {
                    glBindBuffer(GL_ARRAY_BUFFER, bucket->vertex_buffer);
                }
            }

            bucket->vertex_count = 0;

            size_t offset = 0;
            for (auto *processed : bucket->objects) {
                if (processed == NULL) {
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

                offset += processed->staging_buffer_size;

                bucket->vertex_count += processed->vertex_count;
            }

            if (!AGLET_GL_ARB_direct_state_access) {
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glBindVertexArray(bucket->vertex_array); //TODO: fix parameter
            }

            bucket->needs_rebuild = false;

            it++;
        }
    }
}
