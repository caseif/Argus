// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "internal/lowlevel/logging.hpp"

// module resman
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

// module render
#include "argus/render/common/material.hpp"
#include "argus/render/common/transform.hpp"
#include "argus/render/common/vertex.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"
#include "internal/render/pimpl/common/material.hpp"
#include "internal/render/pimpl/2d/render_prim_2d.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"

// module render_opengl
#include "internal/render_opengl/defines.hpp"
#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/renderer/2d/object_proc.hpp"
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
                [](const size_t acc, const RenderPrim2D &prim) {
                    return acc + prim.get_vertex_count();
                }
        );
    }

    void process_object_2d(Scene2DState &scene_state, const RenderObject2D &object, const mat4_flat_t &transform) {
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

        auto existing_it = scene_state.processed_objs.find(&object);
        if (existing_it != scene_state.processed_objs.end()) {
            //TODO: what the hell does this comment mean?
            // for some reason freeing the object before we replace it causes
            // weird issues that seem like a race condition somehow
            auto &old_obj = *existing_it->second;

            glDeleteBuffers(1, &old_obj.vertex_buffer);
            old_obj.~ProcessedRenderObject();

            // the bucket should always exist if the object existed previously
            auto *bucket = scene_state.render_buckets[processed_obj.material_res.uid];
            _ARGUS_ASSERT(!bucket->objects.empty(), "Bucket for existing object should not be empty");
            std::replace(bucket->objects.begin(), bucket->objects.end(), &old_obj, &processed_obj);
            existing_it->second = &processed_obj;
        } else {
            scene_state.processed_objs.insert({ &object, &processed_obj });

            RenderBucket *bucket;
            auto existing_bucket_it = scene_state.render_buckets.find(processed_obj.material_res.uid);
            if (existing_bucket_it != scene_state.render_buckets.end()) {
                bucket = existing_bucket_it->second;
            } else {
                bucket = &RenderBucket::create(processed_obj.material_res);
                scene_state.render_buckets[processed_obj.material_res.uid] = bucket;
            }

            bucket->objects.push_back(&processed_obj);
            bucket->needs_rebuild = true;
        }

        object.get_transform().pimpl->dirty = false;
    }

    void deinit_object_2d(ProcessedRenderObject &obj) {
        glDeleteBuffers(1, &obj.vertex_buffer);
    }
}
