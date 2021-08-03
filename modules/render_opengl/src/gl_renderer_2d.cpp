/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "internal/core/core_util.hpp"

// module resman
#include "argus/resman.hpp"

// module render
#include "argus/render/common/material.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_layer_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "internal/render/pimpl/common/material.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"
#include "internal/render/pimpl/2d/render_group_2d.hpp"
#include "internal/render/pimpl/2d/render_layer_2d.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"
#include "internal/render/pimpl/2d/render_prim_2d.hpp"

// module render_opengl
#include "internal/render_opengl/gl_renderer_2d.hpp"
#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/globals.hpp"
#include "internal/render_opengl/layer_state.hpp"
#include "internal/render_opengl/processed_render_object.hpp"
#include "internal/render_opengl/render_bucket.hpp"
#include "internal/render_opengl/renderer_state.hpp"

#include "aglet/aglet.h"

#include <algorithm>
#include <iterator>
#include <numeric>

#include <cstddef>

namespace argus {
    static size_t _count_vertices(const RenderObject2D &obj) {
        return std::accumulate(obj.get_primitives().cbegin(), obj.get_primitives().cend(), 0,
                [](const size_t acc, const RenderPrim2D &prim) {
                    return acc + prim.get_vertex_count();
                }
        );
    }

    static void _fill_buckets_2d(Layer2DState &layer_state) {
        for (auto it = layer_state.render_buckets.begin(); it != layer_state.render_buckets.end();) {
            auto *bucket = it->second;

            if (bucket->objects.empty()) {
                try_delete_buffer(it->second->vertex_array);
                try_delete_buffer(it->second->vertex_buffer);
                it->second->~RenderBucket();

                it = layer_state.render_buckets.erase(it);

                continue;
            }

            if (bucket->needs_rebuild) {
                if (bucket->vertex_array != 0) {
                    glDeleteVertexArrays(1, &bucket->vertex_array);
                }

                if (bucket->vertex_buffer != 0) {
                    glDeleteBuffers(1, &bucket->vertex_buffer);
                }

                glGenVertexArrays(1, &bucket->vertex_array);
                glBindVertexArray(bucket->vertex_array);

                glGenBuffers(1, &bucket->vertex_buffer);
                glBindBuffer(GL_ARRAY_BUFFER, bucket->vertex_buffer);

                size_t size = 0;
                for (auto &obj : bucket->objects) {
                    size += obj->vertex_buffer_size;
                }

                glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);

                size_t offset = 0;
                for (auto *processed : bucket->objects) {
                    glBindBuffer(GL_COPY_READ_BUFFER, processed->vertex_buffer);
                    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, offset, processed->vertex_buffer_size);
                    glBindBuffer(GL_COPY_READ_BUFFER, 0);

                    offset += processed->vertex_buffer_size;
                }

                auto &material = bucket->material;

                auto vertex_attrs = material.pimpl->attributes;

                size_t vertex_len = ((vertex_attrs & VertexAttributes::POSITION) ? SHADER_ATTRIB_IN_POSITION_LEN : 0)
                        + ((vertex_attrs & VertexAttributes::NORMAL) ? SHADER_ATTRIB_IN_NORMAL_LEN : 0)
                        + ((vertex_attrs & VertexAttributes::COLOR) ? SHADER_ATTRIB_IN_COLOR_LEN : 0)
                        + ((vertex_attrs & VertexAttributes::TEXCOORD) ? SHADER_ATTRIB_IN_TEXCOORD_LEN : 0);

                GLuint attr_offset = 0;

                if (vertex_attrs & VertexAttributes::POSITION) {
                    set_attrib_pointer(vertex_len, SHADER_ATTRIB_IN_POSITION_LEN, SHADER_ATTRIB_LOC_POSITION, &attr_offset);
                }
                if (vertex_attrs & VertexAttributes::NORMAL) {
                    set_attrib_pointer(vertex_len, SHADER_ATTRIB_IN_NORMAL_LEN, SHADER_ATTRIB_LOC_NORMAL, &attr_offset);
                }
                if (vertex_attrs & VertexAttributes::COLOR) {
                    set_attrib_pointer(vertex_len, SHADER_ATTRIB_IN_COLOR_LEN, SHADER_ATTRIB_LOC_COLOR, &attr_offset);
                }
                if (vertex_attrs & VertexAttributes::TEXCOORD) {
                    set_attrib_pointer(vertex_len, SHADER_ATTRIB_IN_TEXCOORD_LEN, SHADER_ATTRIB_LOC_TEXCOORD, &attr_offset);
                }

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(bucket->vertex_array);

                bucket->needs_rebuild = false;
            } else {
                bucket->vertex_count = 0;

                size_t offset = 0;
                for (auto *processed : bucket->objects) {
                    if (processed->updated) {
                        glBindBuffer(GL_COPY_READ_BUFFER, processed->vertex_buffer);
                        glBindBuffer(GL_COPY_WRITE_BUFFER, bucket->vertex_buffer);

                        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, offset,
                                processed->vertex_buffer_size);

                        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
                        glBindBuffer(GL_COPY_READ_BUFFER, 0);
                    }

                    offset += processed->vertex_buffer_size;

                    bucket->vertex_count += processed->vertex_count;
                }
            }

            it++;
        }
    }
    
    static void _process_object_2d(Layer2DState &layer_state,
            const RenderObject2D &object, const mat4_flat_t &transform) {
        size_t vertex_count = 0;
        for (const RenderPrim2D &prim : object.get_primitives()) {
            vertex_count += prim.get_vertex_count();
        }

        size_t buffer_size = vertex_count * _VERTEX_LEN * sizeof(GLfloat);

        buffer_handle_t vertex_buffer;
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_COPY_READ_BUFFER, vertex_buffer);
        glBufferData(GL_COPY_READ_BUFFER, buffer_size, nullptr, GL_DYNAMIC_DRAW);
        auto mapped_buffer = static_cast<GLfloat*>(glMapBuffer(GL_COPY_READ_BUFFER, GL_WRITE_ONLY));

        auto &mat_res = ResourceManager::get_global_resource_manager().get_resource(object.get_material());
        auto &mat = mat_res.get<Material>();

        auto vertex_attrs = mat.pimpl->attributes;

        size_t vertex_len = ((vertex_attrs & VertexAttributes::POSITION) ? SHADER_ATTRIB_IN_POSITION_LEN : 0)
                + ((vertex_attrs & VertexAttributes::NORMAL) ? SHADER_ATTRIB_IN_NORMAL_LEN : 0)
                + ((vertex_attrs & VertexAttributes::COLOR) ? SHADER_ATTRIB_IN_COLOR_LEN : 0)
                + ((vertex_attrs & VertexAttributes::TEXCOORD) ? SHADER_ATTRIB_IN_TEXCOORD_LEN : 0);

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

        glUnmapBuffer(GL_COPY_READ_BUFFER);
        glBindBuffer(GL_COPY_READ_BUFFER, 0);

        auto &processed_obj = ProcessedRenderObject::create(
                mat_res, transform,
                vertex_buffer, buffer_size, _count_vertices(object));
        processed_obj.visited = true;

        auto existing_it = layer_state.processed_objs.find(&object);
        if (existing_it != layer_state.processed_objs.end()) {
            // for some reason freeing the object before we replace it causes
            // weird issues that seem like a race condition somehow
            auto *old_obj = existing_it->second;

            existing_it->second = &processed_obj;

            old_obj->~ProcessedRenderObject();

            // the bucket should always exist if the object existed previously
            auto *bucket = layer_state.render_buckets[processed_obj.material];
            _ARGUS_ASSERT(!bucket->objects.empty(), "Bucket for existing object should not be empty");
            std::replace(bucket->objects.begin(), bucket->objects.end(), existing_it->second, &processed_obj);
        } else {
            layer_state.processed_objs.insert({ &object, &processed_obj });

            RenderBucket *bucket;
            auto existing_bucket_it = layer_state.render_buckets.find(processed_obj.material);
            if (existing_bucket_it != layer_state.render_buckets.end()) {
                bucket = existing_bucket_it->second;
            } else {
                bucket = &RenderBucket::create(*processed_obj.material);
                layer_state.render_buckets[processed_obj.material] = bucket;
            }

            bucket->objects.push_back(&processed_obj);
            bucket->needs_rebuild = true;
        }

        object.get_transform().pimpl->dirty = false;
    }

    static void _compute_abs_group_transform(const RenderGroup2D &group, mat4_flat_t &target) {
        group.get_transform().copy_matrix(target);
        const RenderGroup2D *cur = nullptr;
        const RenderGroup2D *parent = group.get_parent_group();

        while (parent != nullptr) {
            cur = parent;
            parent = parent->get_parent_group();

            mat4_flat_t new_transform;

            multiply_matrices(cur->get_transform().as_matrix(), target, new_transform);

            memcpy(target, new_transform, 16 * sizeof(target[0]));
        }
    }

    static void _process_render_group_2d(RendererState &state, Layer2DState &layer_state, const RenderGroup2D &group,
            const bool recompute_transform, const mat4_flat_t running_transform) {
        bool new_recompute_transform = recompute_transform;
        mat4_flat_t cur_transform;

        if (recompute_transform) {
            // we already know we have to recompute the transform of this whole
            // branch since a parent was dirty
            _ARGUS_ASSERT(running_transform != nullptr, "running_transform is null\n");
            multiply_matrices(running_transform, group.get_transform().as_matrix(), cur_transform);

            new_recompute_transform = true;
        } else if (group.get_transform().pimpl->dirty) {
            _compute_abs_group_transform(group, cur_transform);

            new_recompute_transform = true;

            group.get_transform().pimpl->dirty = false;
        }

        for (const RenderObject2D *child_object : group.pimpl->child_objects) {
            mat4_flat_t final_obj_transform;

            auto existing_it = layer_state.processed_objs.find(child_object);
            // if the object has already been processed previously
            if (existing_it != layer_state.processed_objs.end()) {
                // if a parent group or the object itself has had its transform updated
                existing_it->second->updated = new_recompute_transform || child_object->get_transform().pimpl->dirty;
                existing_it->second->visited = true;
            }

            if (new_recompute_transform) {
                multiply_matrices(cur_transform, child_object->get_transform().as_matrix(), final_obj_transform);
            } else if (child_object->get_transform().pimpl->dirty) {
                // parent transform hasn't been computed so we need to do it here
                mat4_flat_t group_abs_transform;
                _compute_abs_group_transform(group, group_abs_transform);

                multiply_matrices(group_abs_transform, child_object->get_transform().as_matrix(), final_obj_transform);
            } else {
                // nothing else to do if object and all parent groups aren't dirty
                return;
            }

            _process_object_2d(layer_state, *child_object, final_obj_transform);
        }

        for (auto *child_group : group.pimpl->child_groups) {
            _process_render_group_2d(state, layer_state, *child_group, new_recompute_transform, cur_transform);
        }
    }

    static void _process_objects_2d(RendererState &state, Layer2DState &layer_state,
            const RenderLayer2D &layer) {
        _process_render_group_2d(state, layer_state, layer.pimpl->root_group, false, nullptr);

        for (auto it = layer_state.processed_objs.begin(); it != layer_state.processed_objs.end();) {
            auto *processed_obj = it->second;
            if (!processed_obj->visited) {
                // wasn't visited this iteration, must not be present in the scene graph anymore

                glDeleteBuffers(1, &processed_obj->vertex_buffer);

                // we need to remove it from its containing bucket and flag the bucket for a rebuild
                auto *bucket = layer_state.render_buckets[it->second->material];
                remove_from_vector(bucket->objects, processed_obj);
                bucket->needs_rebuild = true;

                processed_obj->~ProcessedRenderObject();

                it = layer_state.processed_objs.erase(it);

                continue;
            }

            it->second->visited = false;
            ++it;
        }
    }

    void render_layer_2d(RenderLayer2D &layer, RendererState &renderer_state, Layer2DState &layer_state) {
        _process_objects_2d(renderer_state, layer_state, layer);
        _fill_buckets_2d(layer_state);
    }
}
