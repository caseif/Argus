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

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/math.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/material.hpp"
#include "argus/render/common/transform.hpp"
#include "argus/render/common/vertex.hpp"
#include "argus/render/util/object_processor.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"

#include "internal/render_opengles/defines.hpp"
#include "internal/render_opengles/types.hpp"
#include "internal/render_opengles/renderer/shader_mgmt.hpp"
#include "internal/render_opengles/renderer/2d/object_proc_impl.hpp"
#include "internal/render_opengles/state/processed_render_object.hpp"
#include "internal/render_opengles/state/render_bucket.hpp"
#include "internal/render_opengles/state/renderer_state.hpp"
#include "internal/render_opengles/state/scene_state.hpp"

#include "aglet/aglet.h"

#include <algorithm>
#include <cstddef>
#include <atomic>
#include <map>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace argus {
    static size_t _count_vertices(const RenderObject2D &obj) {
        return std::accumulate(obj.get_primitives().cbegin(), obj.get_primitives().cend(), std::size_t{ 0 },
                [](const auto acc, const auto &prim) {
                    return acc + prim.get_vertex_count();
                }
        );
    }

    ProcessedRenderObject2DPtr create_processed_object_2d(const RenderObject2D &object, const Matrix4 &transform,
            void *scene_state_ptr) {
        auto &scene_state = *static_cast<SceneState*>(scene_state_ptr);
        auto &state = scene_state.parent_state;

        size_t vertex_count = 0;
        for (const RenderPrim2D &prim : object.get_primitives()) {
            vertex_count += prim.get_vertex_count();
        }

        const auto &mat_res = ResourceManager::instance().get_resource(object.get_material());

        if (state.linked_programs.find(object.get_material()) == state.linked_programs.end()) {
            build_shaders(state, mat_res);
        }
        auto &program = state.linked_programs.find(object.get_material())->second;

        size_t vertex_len = (program.attr_position_loc.has_value() ? SHADER_ATTRIB_IN_POSITION_LEN : 0)
                + (program.attr_normal_loc.has_value() ? SHADER_ATTRIB_IN_NORMAL_LEN : 0)
                + (program.attr_color_loc.has_value()? SHADER_ATTRIB_IN_COLOR_LEN : 0)
                + (program.attr_texcoord_loc.has_value() ? SHADER_ATTRIB_IN_TEXCOORD_LEN : 0);

        size_t buffer_size = vertex_count * vertex_len * sizeof(GLfloat);

        buffer_handle_t vertex_buffer;
        GLfloat *mapped_buffer;
        bool persistent_buffer = false;

        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_COPY_READ_BUFFER, vertex_buffer);
        glBufferData(GL_COPY_READ_BUFFER, buffer_size, nullptr, GL_DYNAMIC_DRAW);
        mapped_buffer = static_cast<GLfloat*>(glMapBufferRange(GL_COPY_READ_BUFFER, 0, buffer_size, GL_MAP_WRITE_BIT));

        size_t total_vertices = 0;
        for (const RenderPrim2D &prim : object.get_primitives()) {
            for (auto &vertex : prim.get_vertices()) {
                size_t major_off = total_vertices * vertex_len;
                size_t minor_off = 0;

                if (program.attr_position_loc.has_value()) {
                    auto transformed_pos = multiply_matrix_and_vector(vertex.position, transform);
                    mapped_buffer[major_off + minor_off++] = transformed_pos.x;
                    mapped_buffer[major_off + minor_off++] = transformed_pos.y;
                }
                if (program.attr_normal_loc.has_value()) {
                    mapped_buffer[major_off + minor_off++] = vertex.normal.x;
                    mapped_buffer[major_off + minor_off++] = vertex.normal.y;
                }
                if (program.attr_color_loc.has_value()) {
                    mapped_buffer[major_off + minor_off++] = vertex.color.r;
                    mapped_buffer[major_off + minor_off++] = vertex.color.g;
                    mapped_buffer[major_off + minor_off++] = vertex.color.b;
                    mapped_buffer[major_off + minor_off++] = vertex.color.a;
                }
                if (program.attr_texcoord_loc.has_value()) {
                    mapped_buffer[major_off + minor_off++] = vertex.tex_coord.x;
                    mapped_buffer[major_off + minor_off++] = vertex.tex_coord.y;
                }

                total_vertices += 1;
            }
        }

        glUnmapBuffer(GL_COPY_READ_BUFFER);
        glBindBuffer(GL_COPY_READ_BUFFER, 0);

        auto &processed_obj = ProcessedRenderObject::create(
                mat_res, vertex_buffer, buffer_size, _count_vertices(object),
                persistent_buffer ? mapped_buffer : nullptr);
        processed_obj.visited = true;
        processed_obj.newly_created = true;

        return &processed_obj;
    }

    void update_processed_object_2d(const RenderObject2D &object, ProcessedRenderObject2DPtr proc_obj_ptr,
            const Matrix4 &transform, bool is_transform_dirty, void *scene_state_ptr) {
        auto &scene_state = *static_cast<SceneState*>(scene_state_ptr);
        auto &state = scene_state.parent_state;

        // program should be linked by now
        auto &program = state.linked_programs.find(object.get_material())->second;

        auto &proc_obj = *reinterpret_cast<ProcessedRenderObject*>(proc_obj_ptr);

        // if a parent group or the object itself has had its transform updated
        proc_obj.updated = is_transform_dirty;

        if (!is_transform_dirty) {
            // nothing to do
            proc_obj.visited = true;
            return;
        }


        size_t vertex_len = (program.attr_position_loc.has_value() ? SHADER_ATTRIB_IN_POSITION_LEN : 0)
                + (program.attr_normal_loc.has_value() ? SHADER_ATTRIB_IN_NORMAL_LEN : 0)
                + (program.attr_color_loc.has_value() ? SHADER_ATTRIB_IN_COLOR_LEN : 0)
                + (program.attr_texcoord_loc.has_value() ? SHADER_ATTRIB_IN_TEXCOORD_LEN : 0);

        GLfloat *mapped_buffer;

        size_t vertex_count = 0;
        for (const RenderPrim2D &prim : object.get_primitives()) {
            vertex_count += prim.get_vertex_count();
        }

        size_t buffer_size = vertex_count * vertex_len * sizeof(GLfloat);

        if (proc_obj.mapped_buffer != nullptr) {
            mapped_buffer = static_cast<GLfloat*>(proc_obj.mapped_buffer);
        } else {
            glBindBuffer(GL_COPY_READ_BUFFER, proc_obj.staging_buffer);
            mapped_buffer = static_cast<GLfloat*>(glMapBufferRange(GL_COPY_READ_BUFFER, 0, buffer_size, GL_MAP_WRITE_BIT));
        }

        size_t total_vertices = 0;
        for (const RenderPrim2D &prim : object.get_primitives()) {
            for (auto &vertex : prim.get_vertices()) {
                size_t major_off = total_vertices * vertex_len;
                size_t minor_off = 0;

                auto transformed_pos = multiply_matrix_and_vector(vertex.position, transform);
                mapped_buffer[major_off + minor_off++] = transformed_pos.x;
                mapped_buffer[major_off + minor_off++] = transformed_pos.y;

                total_vertices += 1;
            }
        }

        if (proc_obj.mapped_buffer == nullptr) {
            glUnmapBuffer(GL_COPY_READ_BUFFER);
            glBindBuffer(GL_COPY_READ_BUFFER, 0);
        }

        proc_obj.visited = true;
        proc_obj.updated = true;
    }

    void deinit_object_2d(ProcessedRenderObject &obj) {
        if (obj.mapped_buffer != nullptr) {
            glBindBuffer(GL_ARRAY_BUFFER, obj.staging_buffer);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        glDeleteBuffers(1, &obj.staging_buffer);
    }
}
