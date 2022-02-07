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

#include "argus/lowlevel/math.hpp"
#include "internal/lowlevel/logging.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/material.hpp"
#include "argus/render/common/transform.hpp"
#include "argus/render/common/vertex.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"
#include "internal/render/pimpl/common/material.hpp"
#include "internal/render/pimpl/2d/render_prim_2d.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"
#include "internal/render/util/object_processor.hpp"

#include "internal/render_opengl/defines.hpp"
#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/renderer/2d/object_proc_impl.hpp"
#include "internal/render_opengl/state/processed_render_object.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"
#include "internal/render_opengl/state/scene_state.hpp"

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
        return std::accumulate(obj.get_primitives().cbegin(), obj.get_primitives().cend(), 0,
                [](const auto acc, const auto &prim) {
                    return acc + prim.get_vertex_count();
                }
        );
    }

    ProcessedRenderObject2DPtr create_processed_object_2d(const RenderObject2D &object,
            const Matrix4 &transform) {
        size_t vertex_count = 0;
        for (const RenderPrim2D &prim : object.get_primitives()) {
            vertex_count += prim.get_vertex_count();
        }

        const auto &mat_res = ResourceManager::instance().get_resource(object.get_material());
        auto &mat = mat_res.get<Material>();

        auto vertex_attrs = mat.pimpl->attributes;

        size_t vertex_len = ((vertex_attrs & VertexAttributes::POSITION) ? SHADER_ATTRIB_IN_POSITION_LEN : 0)
                + ((vertex_attrs & VertexAttributes::NORMAL) ? SHADER_ATTRIB_IN_NORMAL_LEN : 0)
                + ((vertex_attrs & VertexAttributes::COLOR) ? SHADER_ATTRIB_IN_COLOR_LEN : 0)
                + ((vertex_attrs & VertexAttributes::TEXCOORD) ? SHADER_ATTRIB_IN_TEXCOORD_LEN : 0);

        size_t buffer_size = vertex_count * vertex_len * sizeof(GLfloat);

        buffer_handle_t vertex_buffer;
        GLfloat *mapped_buffer;
        bool persistent_buffer = false;

        if (AGLET_GL_ARB_direct_state_access) {
            glCreateBuffers(1, &vertex_buffer);
            if (AGLET_GL_ARB_buffer_storage) {
                glNamedBufferStorage(vertex_buffer, buffer_size, nullptr, GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
                persistent_buffer = true;
                mapped_buffer = static_cast<GLfloat*>(glMapNamedBufferRange(vertex_buffer, 0, buffer_size,
                        GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT));
            } else {
                glNamedBufferData(vertex_buffer, buffer_size, nullptr, GL_DYNAMIC_DRAW);
                mapped_buffer = static_cast<GLfloat*>(glMapNamedBuffer(vertex_buffer, GL_WRITE_ONLY));
            }
        } else {
            glGenBuffers(1, &vertex_buffer);
            glBindBuffer(GL_COPY_READ_BUFFER, vertex_buffer);
            glBufferData(GL_COPY_READ_BUFFER, buffer_size, nullptr, GL_DYNAMIC_DRAW);
            mapped_buffer = static_cast<GLfloat*>(glMapBuffer(GL_COPY_READ_BUFFER, GL_WRITE_ONLY));
        }

        size_t total_vertices = 0;
        for (const RenderPrim2D &prim : object.get_primitives()) {
            for (size_t i = 0; i < prim.pimpl->vertices.size(); i++) {
                const Vertex2D &vertex = prim.pimpl->vertices.at(i);
                size_t major_off = total_vertices * vertex_len;
                size_t minor_off = 0;

                if (vertex_attrs & VertexAttributes::POSITION) {
                    auto transformed_pos = multiply_matrix_and_vector(vertex.position, transform);
                    mapped_buffer[major_off + minor_off++] = transformed_pos.x;
                    mapped_buffer[major_off + minor_off++] = transformed_pos.y;
                }
                if (vertex_attrs & VertexAttributes::NORMAL) {
                    mapped_buffer[major_off + minor_off++] = vertex.normal.x;
                    mapped_buffer[major_off + minor_off++] = vertex.normal.y;
                }
                if (vertex_attrs & VertexAttributes::COLOR) {
                    mapped_buffer[major_off + minor_off++] = vertex.color.r;
                    mapped_buffer[major_off + minor_off++] = vertex.color.g;
                    mapped_buffer[major_off + minor_off++] = vertex.color.b;
                    mapped_buffer[major_off + minor_off++] = vertex.color.a;
                }
                if (vertex_attrs & VertexAttributes::TEXCOORD) {
                    mapped_buffer[major_off + minor_off++] = vertex.tex_coord.x;
                    mapped_buffer[major_off + minor_off++] = vertex.tex_coord.y;
                }

                total_vertices += 1;
            }
        }

        if (!AGLET_GL_ARB_direct_state_access) {
            glUnmapBuffer(GL_COPY_READ_BUFFER);
            glBindBuffer(GL_COPY_READ_BUFFER, 0);
        }

        auto &processed_obj = ProcessedRenderObject::create(
                mat_res, vertex_buffer, buffer_size, _count_vertices(object),
                persistent_buffer ? mapped_buffer : nullptr);
        processed_obj.visited = true;
        processed_obj.newly_created = true;

        return &processed_obj;
    }

    void update_processed_object_2d(const RenderObject2D &object,
            ProcessedRenderObject2DPtr proc_obj_ptr, const Matrix4 &transform, bool is_transform_dirty) {
        auto &proc_obj = *reinterpret_cast<ProcessedRenderObject*>(proc_obj_ptr);

        // if a parent group or the object itself has had its transform updated
        proc_obj.updated = is_transform_dirty;

        auto vertex_attrs = proc_obj.material_res.get<Material>().pimpl->attributes;
        // not sure whether this would actually ever be false in practice
        if (vertex_attrs & VertexAttributes::POSITION) {
            size_t vertex_len = ((vertex_attrs & VertexAttributes::POSITION) ? SHADER_ATTRIB_IN_POSITION_LEN : 0)
                    + ((vertex_attrs & VertexAttributes::NORMAL) ? SHADER_ATTRIB_IN_NORMAL_LEN : 0)
                    + ((vertex_attrs & VertexAttributes::COLOR) ? SHADER_ATTRIB_IN_COLOR_LEN : 0)
                    + ((vertex_attrs & VertexAttributes::TEXCOORD) ? SHADER_ATTRIB_IN_TEXCOORD_LEN : 0);

            GLfloat *mapped_buffer;

            if (proc_obj.mapped_buffer != nullptr) {
                mapped_buffer = static_cast<GLfloat*>(proc_obj.mapped_buffer);
            } else if (AGLET_GL_ARB_direct_state_access) {
                mapped_buffer = static_cast<GLfloat*>(glMapNamedBuffer(proc_obj.staging_buffer, GL_WRITE_ONLY));
            } else {
                glBindBuffer(GL_COPY_READ_BUFFER, proc_obj.staging_buffer);
                mapped_buffer = static_cast<GLfloat*>(glMapBuffer(GL_COPY_READ_BUFFER, GL_WRITE_ONLY));
            }

            size_t total_vertices = 0;
            for (const RenderPrim2D &prim : object.get_primitives()) {
                for (size_t i = 0; i < prim.pimpl->vertices.size(); i++) {
                    const Vertex2D &vertex = prim.pimpl->vertices.at(i);
                    size_t major_off = total_vertices * vertex_len;
                    size_t minor_off = 0;

                    auto transformed_pos = multiply_matrix_and_vector(vertex.position, transform);
                    mapped_buffer[major_off + minor_off++] = transformed_pos.x;
                    mapped_buffer[major_off + minor_off++] = transformed_pos.y;

                    total_vertices += 1;
                }
            }

            if (proc_obj.mapped_buffer == nullptr) {
                if (AGLET_GL_ARB_direct_state_access) {
                    glUnmapNamedBuffer(proc_obj.staging_buffer);
                } else {
                    glUnmapBuffer(GL_COPY_READ_BUFFER);
                    glBindBuffer(GL_COPY_READ_BUFFER, 0);
                }
            }
        }

        proc_obj.visited = true;
        proc_obj.updated = true;
    }

    void deinit_object_2d(ProcessedRenderObject &obj) {
        if (obj.mapped_buffer != nullptr) {
            if (AGLET_GL_ARB_direct_state_access) {
                glUnmapNamedBuffer(obj.staging_buffer);
            } else {
                glBindBuffer(GL_ARRAY_BUFFER, obj.staging_buffer);
                glUnmapBuffer(GL_ARRAY_BUFFER);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }
        glDeleteBuffers(1, &obj.staging_buffer);
    }
}