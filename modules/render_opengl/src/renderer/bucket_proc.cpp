// module resman
#include "argus/resman/resource.hpp"

// module render
#include "argus/render/common/material.hpp"
#include "internal/render/pimpl/common/material.hpp"

// module render_opengl
#include "internal/render_opengl/defines.hpp"
#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/renderer/bucket_proc.hpp"
#include "internal/render_opengl/state/processed_render_object.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"
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

                auto &material = bucket->material_res.get<Material>();

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
}