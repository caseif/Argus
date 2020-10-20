/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"
#include "argus/lowlevel/threading.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core/core_util.hpp"

// module render
#include "argus/render/material.hpp"
#include "argus/render/render_group.hpp"
#include "argus/render/render_layer.hpp"
#include "argus/render/render_object.hpp"
#include "argus/render/render_prim.hpp"
#include "argus/render/renderer.hpp"
#include "argus/render/shader.hpp"
#include "argus/render/texture_data.hpp"
#include "argus/render/transform.hpp"
#include "argus/render/window.hpp"
#include "internal/render/pimpl/material.hpp"
#include "internal/render/pimpl/render_group.hpp"
#include "internal/render/pimpl/render_layer.hpp"
#include "internal/render/pimpl/render_prim.hpp"
#include "internal/render/pimpl/renderer.hpp"
#include "internal/render/pimpl/shader.hpp"
#include "internal/render/pimpl/texture_data.hpp"
#include "internal/render/pimpl/transform.hpp"
#include "internal/render/pimpl/window.hpp"
#include "internal/render/renderer_impl.hpp"

// module render_opengl
#include "internal/render_opengl/gl_renderer.hpp"
#include "internal/render_opengl/glext.hpp"
#include "internal/render_opengl/glfw_include.hpp"
#include "internal/render_opengl/globals.hpp"
#include "internal/render_opengl/processed_render_object.hpp"
#include "internal/render_opengl/render_bucket.hpp"
#include "internal/render_opengl/renderer_state.hpp"

#include <algorithm>
#include <atomic>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <cstdio>
#include <cstring>

namespace argus {
    static std::map<const Renderer*, RendererState> g_renderer_states;

    static AllocPool g_obj_pool(sizeof(ProcessedRenderObject));
    static AllocPool g_bucket_pool(sizeof(RenderBucket));

    static void _activate_gl_context(GLFWwindow *window) {
        if (glfwGetCurrentContext() == window) {
            // already current
            return;
        }

        glfwMakeContextCurrent(window);
        if (glfwGetCurrentContext() != window) {
            _ARGUS_FATAL("Failed to make GL context current\n");
        }
    }

    static void APIENTRY _gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                            const GLchar *message, const void *userParam) {
#ifndef _ARGUS_DEBUG_MODE
        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION || severity == GL_DEBUG_SEVERITY_LOW) {
            return;
        }
#endif
        char const *level;
        auto stream = stdout;
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                level = "SEVERE";
                stream = stderr;
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                level = "WARN";
                stream = stderr;
                break;
            case GL_DEBUG_SEVERITY_LOW:
                level = "INFO";
                break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                level = "TRACE";
                break;
        }
        _GENERIC_PRINT(stream, level, "GL", "%s\n", message);
    }

    GLRenderer::GLRenderer(void): RendererImpl() {
    }

    void GLRenderer::init_context_hints(void) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        #ifdef _ARGUS_DEBUG_MODE
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        #endif
    }

    // we do the init in a separate method so the GL context is always created from the render thread
    void GLRenderer::init(Renderer &renderer) {
        _activate_gl_context(renderer.pimpl->window.pimpl->handle);

        init_opengl_extensions();

        int gl_major;
        int gl_minor;
        glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
        glGetIntegerv(GL_MINOR_VERSION, &gl_minor);
        if (gl_major < 3 || (gl_major == 3 && gl_minor < 3)) {
            _ARGUS_FATAL("Argus requires support for OpenGL 3.3 or higher\n");
        }

        _ARGUS_INFO("Obtained OpenGL %d.%d context\n", gl_major, gl_minor);

        const GLubyte *ver_str = glGetString(GL_VERSION);

        //TODO: actually do something
        if (glDebugMessageCallback != nullptr) {
            glGetError();
            glDebugMessageCallback(_gl_debug_callback, nullptr);
        } else {
        }

        glDepthFunc(GL_ALWAYS);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDisable(GL_CULL_FACE);
        
        g_renderer_states[&renderer] = {};
    }

    static void _delete_bucket(RenderBucket *const bucket) {
        if (bucket->vertex_array != 0) {
            glDeleteBuffers(1, &bucket->vertex_array);
        }

        if (bucket->vertex_buffer != 0) {
            glDeleteBuffers(1, &bucket->vertex_buffer);
        }

        g_bucket_pool.free(bucket);
    }

    void GLRenderer::deinit_texture(const TextureData &texture) {
        for (auto &state : g_renderer_states) {
            auto &textures = state.second.prepared_textures;
            auto existing = textures.find(&texture);
            if (existing != textures.end()) {
                glDeleteTextures(1, &existing->second);
                textures.erase(existing);
            }
        }
    }

    void GLRenderer::deinit_shader(const Shader &shader) {
        for (auto &state : g_renderer_states) {
            auto &shaders = state.second.compiled_shaders;
            auto existing = shaders.find(&shader);
            if (existing != shaders.end()) {
                glDeleteShader(existing->second);
                shaders.erase(existing);
            }
        }
    }

    void GLRenderer::deinit_material(const Material &material) {
        for (auto &state : g_renderer_states) {
            for (auto &layer_state : state.second.layer_states) {
                auto &buckets = layer_state.second.render_buckets;
                auto bucket = buckets.find(&material);
                if (bucket != buckets.end()) {
                    _delete_bucket(bucket->second);
                    buckets.erase(bucket);
                }
            }

            auto &programs = state.second.linked_programs;
            auto program = programs.find(&material);
            if (program != programs.end()) {
                glDeleteProgram(program->second.handle);
            }

            programs.erase(program);
        }
    }

    static void _process_object(const RenderObject &object, const mat4_flat_t &transform) {
        auto &layer = object.get_parent_layer();
        auto &renderer = layer.get_parent_renderer();

        auto &state = g_renderer_states[&renderer];
        auto &layer_state = state.layer_states[&layer];

        auto existing = layer_state.processed_objs.find(&object);

        size_t vertex_count = 0;
        for (const RenderPrim &prim : object.get_primitives()) {
            vertex_count += prim.get_vertex_count();
        }

        size_t buffer_size = vertex_count * _VERTEX_LEN * sizeof(GLfloat);

        buffer_handle_t vertex_buffer;
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_COPY_READ_BUFFER, vertex_buffer);
        glBufferData(GL_COPY_READ_BUFFER, buffer_size, nullptr, GL_DYNAMIC_DRAW);
        auto mapped_buffer = static_cast<GLfloat*>(glMapBuffer(GL_COPY_READ_BUFFER, GL_WRITE_ONLY));

        auto vertex_attrs = object.get_material().pimpl->attributes;

        size_t vertex_len = ((vertex_attrs & VertexAttributes::POSITION) ? SHADER_ATTRIB_IN_POSITION_LEN : 0)
                + ((vertex_attrs & VertexAttributes::NORMAL) ? SHADER_ATTRIB_IN_NORMAL_LEN : 0)
                + ((vertex_attrs & VertexAttributes::COLOR) ? SHADER_ATTRIB_IN_COLOR_LEN : 0)
                + ((vertex_attrs & VertexAttributes::TEXCOORD) ? SHADER_ATTRIB_IN_TEXCOORD_LEN : 0);

        size_t total_vertices = 0;
        for (const RenderPrim &prim : object.get_primitives()) {
            for (size_t i = 0; i < prim.pimpl->vertices.size(); i++) {
                const Vertex &vertex = prim.pimpl->vertices.at(i);
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

        auto &processed_obj = g_obj_pool.construct<ProcessedRenderObject>(
                object, object.get_material(), transform,
                vertex_buffer, buffer_size);
        
        if (existing != layer_state.processed_objs.end()) {
            g_obj_pool.free(existing->second);
            existing->second = &processed_obj;
            processed_obj.visited = true;

            existing->second = &processed_obj;

            // the bucket should always exist if the object existed previously
            auto *bucket = layer_state.render_buckets[processed_obj.material];
            _ARGUS_ASSERT(!bucket->objects.empty(), "Bucket for existing object should not be empty");
            std::replace(bucket->objects.begin(), bucket->objects.end(), existing->second, &processed_obj);
        } else {
            layer_state.processed_objs.insert({ &object, &processed_obj });

            RenderBucket *bucket;
            auto existing_bucket = layer_state.render_buckets.find(processed_obj.material);
            if (existing_bucket != layer_state.render_buckets.end()) {
                bucket = existing_bucket->second;
            } else {
                bucket = &g_bucket_pool.construct<RenderBucket>(*processed_obj.material);
                layer_state.render_buckets[processed_obj.material] = bucket;
            }

            bucket->objects.push_back(&processed_obj);
            bucket->needs_rebuild = true;
        }
    }

    static void _compute_abs_group_transform(const RenderGroup &group, mat4_flat_t target) {
        group.get_transform().copy_matrix(target);
        const RenderGroup *cur = &group;
        const RenderGroup *parent = group.get_parent_group();

        while (parent != nullptr) {
            cur = parent;
            parent = parent->get_parent_group();

            mat4_flat_t new_transform;

            multiply_matrices(cur->get_transform().as_matrix(), target, new_transform);

            memcpy(target, new_transform, 16 * sizeof(target[0]));
        }
    }

    static void _process_render_group(const RenderGroup &group, const bool recompute_transform,
            const mat4_flat_t running_transform) {
        auto &layer = group.get_parent_layer();
        auto &renderer = layer.get_parent_renderer();

        auto &state = g_renderer_states[&renderer];
        auto &layer_state = state.layer_states[&layer];

        bool new_recompute_transform = recompute_transform;
        mat4_flat_t cur_transform;

        if (recompute_transform) {
            // we already know we have to recompute the transform of this whole
            // branch since a parent was dirty
            _ARGUS_ASSERT(running_transform != nullptr, "running_transform is null\n");
            multiply_matrices(running_transform, group.get_transform().as_matrix(), cur_transform);

            new_recompute_transform = true;
        } else if (group.get_transform().is_dirty()) {
            _compute_abs_group_transform(group, cur_transform);

            new_recompute_transform = true;

            group.get_transform().pimpl->dirty = false;
        }

        for (const RenderObject *child_object : group.pimpl->child_objects) {
            mat4_flat_t final_obj_transform;

            auto existing = layer_state.processed_objs.find(child_object);
            // if the object has already been processed previously
            if (existing != layer_state.processed_objs.end()) {
                // if a parent group or the object itself has had its transform updated
                existing->second->updated = new_recompute_transform || child_object->get_transform().is_dirty();
                existing->second->visited = true;
            }

            if (new_recompute_transform) {
                multiply_matrices(cur_transform, child_object->get_transform().as_matrix(), final_obj_transform);
            } else if (child_object->get_transform().is_dirty()) {
                // parent transform hasn't been computed so we need to do it here
                mat4_flat_t group_abs_transform;
                _compute_abs_group_transform(group, group_abs_transform);

                multiply_matrices(group_abs_transform, child_object->get_transform().as_matrix(), final_obj_transform);
            } else {
                // nothing else to do if object and all parent groups aren't dirty
                return;
            }

            _process_object(*child_object, final_obj_transform);
        }

        for (auto *child_group : group.pimpl->child_groups) {
            _process_render_group(*child_group, new_recompute_transform, cur_transform);
        }
    }

    static void _process_objects(const Renderer &renderer, const RenderLayer &layer) {
        auto &state = g_renderer_states[&renderer];
        auto &layer_state = state.layer_states[&layer];

        for (auto *layer : renderer.pimpl->render_layers) {
            std::vector<ProcessedRenderObject> processed;
            _process_render_group(layer->pimpl->root_group, false, nullptr);
        }

        for (auto it = layer_state.processed_objs.begin(); it != layer_state.processed_objs.end();) {
            auto *processed_obj = it->second;
            if (!processed_obj->visited) {
                // wasn't visited this iteration, must not be present in the scene graph anymore

                auto buffer = processed_obj->vertex_buffer;
                glDeleteBuffers(1, &processed_obj->vertex_buffer);

                // we need to remove it from its containing bucket and flag the bucket for a rebuild
                auto bucket = layer_state.render_buckets.find(it->second->material);
                if (bucket != layer_state.render_buckets.end()) { // I think this should always be true
                    auto &bucket_objs = bucket->second->objects;
                    remove_from_vector(bucket_objs, processed_obj);
                    bucket->second->needs_rebuild = true;
                }

                g_obj_pool.free(processed_obj);

                it = layer_state.processed_objs.erase(it);

                continue;
            }

            it->second->visited = false;
            ++it;
        }
    }

    inline static void _set_attrib_pointer(GLuint vertex_len, GLuint attr_len, GLuint attr_index, GLuint *attr_offset) {
        glEnableVertexAttribArray(attr_index);
        glVertexAttribPointer(attr_index, attr_len, GL_FLOAT, GL_FALSE, vertex_len * sizeof(GLfloat),
                reinterpret_cast<GLvoid*>(*attr_offset));
        *attr_offset += attr_len * sizeof(GLfloat);
    }

    static void _fill_buckets(const Renderer &renderer, const RenderLayer &layer) {
        auto &state = g_renderer_states[&renderer];
        auto &layer_state = state.layer_states[&layer];

        for (auto it = layer_state.render_buckets.begin(); it != layer_state.render_buckets.end();) {
            auto bucket = it->second;

            if (bucket->objects.empty()) {
                _delete_bucket(it->second);

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
                
                glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);

                size_t offset = 0;
                for (auto *processed : bucket->objects) {
                    glBindBuffer(GL_COPY_READ_BUFFER, processed->vertex_buffer);
                    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, offset, processed->vertex_buffer_size);
                    glBindBuffer(GL_COPY_READ_BUFFER, 0);

                    offset += processed->vertex_buffer_size;
                }
                
                auto &material = bucket->material;
                auto program_handle = state.linked_programs.find(&material)->second;

                auto vertex_attrs = material.pimpl->attributes;

                size_t vertex_len = ((vertex_attrs & VertexAttributes::POSITION) ? SHADER_ATTRIB_IN_POSITION_LEN : 0)
                        + ((vertex_attrs & VertexAttributes::NORMAL) ? SHADER_ATTRIB_IN_NORMAL_LEN : 0)
                        + ((vertex_attrs & VertexAttributes::COLOR) ? SHADER_ATTRIB_IN_COLOR_LEN : 0)
                        + ((vertex_attrs & VertexAttributes::TEXCOORD) ? SHADER_ATTRIB_IN_TEXCOORD_LEN : 0);

                GLuint attr_offset = 0;

                if (vertex_attrs & VertexAttributes::POSITION) {
                    _set_attrib_pointer(vertex_len, SHADER_ATTRIB_IN_POSITION_LEN, SHADER_ATTRIB_LOC_POSITION, &attr_offset);
                }
                if (vertex_attrs & VertexAttributes::NORMAL) {
                    _set_attrib_pointer(vertex_len, SHADER_ATTRIB_IN_NORMAL_LEN, SHADER_ATTRIB_LOC_NORMAL, &attr_offset);
                }
                if (vertex_attrs & VertexAttributes::COLOR) {
                    _set_attrib_pointer(vertex_len, SHADER_ATTRIB_IN_COLOR_LEN, SHADER_ATTRIB_LOC_COLOR, &attr_offset);
                }
                if (vertex_attrs & VertexAttributes::TEXCOORD) {
                    _set_attrib_pointer(vertex_len, SHADER_ATTRIB_IN_TEXCOORD_LEN, SHADER_ATTRIB_LOC_TEXCOORD, &attr_offset);
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

    static void _build_shaders(const Renderer &renderer, const Material &material) {
        auto &state = g_renderer_states[&renderer];

        auto existing_program = state.linked_programs.find(&material);
        if (existing_program != state.linked_programs.end()) {
            return;
        }

        auto program_handle = glCreateProgram();
        if (!glIsProgram(program_handle)) {
            _ARGUS_FATAL("Failed to create program: %d\n", glGetError());
        }

        for (auto *shader : material.pimpl->shaders) {
            shader_handle_t shader_handle;

            auto existing_shader = state.compiled_shaders.find(shader);
            if (existing_shader != state.compiled_shaders.end()) {
                shader_handle = existing_shader->second;
            } else {
                GLuint shader_stage;
                switch (shader->pimpl->stage) {
                    case ShaderStage::VERTEX:
                        shader_stage = GL_VERTEX_SHADER;
                        break;
                    case ShaderStage::FRAGMENT:
                        shader_stage = GL_FRAGMENT_SHADER;
                        break;
                    default:
                        _ARGUS_FATAL("Unrecognized shader stage ordinal %d\n", shader->pimpl->stage);
                }

                shader_handle = glCreateShader(shader_stage);
                if (!glIsShader(shader_handle)) {
                    _ARGUS_FATAL("Failed to create shader: %d\n", glGetError());
                }

                const char *src = shader->pimpl->src;
                const GLint src_len = shader->pimpl->src_len;
                glShaderSource(shader_handle, 1, &src, &src_len);

                glCompileShader(shader_handle);

                int res;
                glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &res);
                if (res == GL_FALSE) {
                    int log_len;
                    glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &log_len);
                    char *log = new char[log_len + 1];
                    glGetShaderInfoLog(shader_handle, log_len, nullptr, log);
                    std::string stage_str;
                    switch (shader->pimpl->stage) {
                        case ShaderStage::VERTEX:
                            stage_str = "vertex";
                            break;
                        case ShaderStage::FRAGMENT:
                            stage_str = "fragment";
                            break;
                        default:
                            stage_str = "unknown";
                            break;
                    }
                    _ARGUS_FATAL("Failed to compile %s shader: %s\n", stage_str.c_str(), log);
                    // fatal crash so we don't need to free the log string
                }

                state.compiled_shaders.insert({ shader, shader_handle });
            }

            glAttachShader(program_handle, shader_handle);
        }

        unsigned int attrib_index = 0;
        if (material.pimpl->attributes & VertexAttributes::POSITION) {
            glBindAttribLocation(program_handle, SHADER_ATTRIB_LOC_POSITION, SHADER_ATTRIB_IN_POSITION);
        }
        if (material.pimpl->attributes & VertexAttributes::NORMAL) {
            glBindAttribLocation(program_handle, SHADER_ATTRIB_LOC_NORMAL, SHADER_ATTRIB_IN_NORMAL);
        }
        if (material.pimpl->attributes & VertexAttributes::COLOR) {
            glBindAttribLocation(program_handle, SHADER_ATTRIB_LOC_COLOR, SHADER_ATTRIB_IN_COLOR);
        }
        if (material.pimpl->attributes & VertexAttributes::TEXCOORD) {
            glBindAttribLocation(program_handle, SHADER_ATTRIB_LOC_TEXCOORD, SHADER_ATTRIB_IN_TEXCOORD);
        }

        glBindFragDataLocation(program_handle, 0, SHADER_ATTRIB_OUT_FRAGDATA);

        glLinkProgram(program_handle);

        int res;
        glGetProgramiv(program_handle, GL_LINK_STATUS, &res);
        if (res == GL_FALSE) {
            int log_len;
            glGetProgramiv(program_handle, GL_INFO_LOG_LENGTH, &log_len);
            char *log = new char[log_len];
            glGetProgramInfoLog(program_handle, GL_INFO_LOG_LENGTH, nullptr, log);
            _ARGUS_FATAL("Failed to link program: %s\n", log);
            delete[] log;
        }

        auto proj_mat_loc = glGetUniformLocation(program_handle, SHADER_UNIFORM_VIEW_MATRIX);

        state.linked_programs[&material] = { program_handle, proj_mat_loc };

        for (auto *shader : material.pimpl->shaders) {
            glDetachShader(program_handle, state.compiled_shaders[shader]);
        }
    }

    static void _prepare_texture(const Renderer &renderer, const Material &material) {
        auto &state = g_renderer_states[&renderer];

        auto &texture = material.pimpl->texture;

        if (state.prepared_textures.find(&texture) != state.prepared_textures.end()) {
            return;
        }

        texture_handle_t handle;

        glGenTextures(1, &handle);

        glBindTexture(GL_TEXTURE_2D, handle);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        size_t row_size = texture.width * 32 / 8;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        size_t offset = 0;
        for (size_t y = 0; y < texture.height; y++) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, texture.width, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                    texture.pimpl->image_data[y]);
            offset += row_size;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        glBindTexture(GL_TEXTURE_2D, 0);

        state.prepared_textures.insert({ &texture, handle });
    }

    static void _rebuild_scene(const Renderer &renderer) {
        auto &state = g_renderer_states[&renderer];

        for (auto *layer : renderer.pimpl->render_layers) {
            _process_objects(renderer, *layer);

            _fill_buckets(renderer, *layer);

            for (auto bucket : state.layer_states[layer].render_buckets) {
                auto &mat = bucket.second->material;

                _build_shaders(renderer, mat);

                _prepare_texture(renderer, mat);
            }
        }
    }

    static void _render_layer(const Renderer &renderer, const RenderLayer &layer) {
        auto &state = g_renderer_states[&renderer];
        auto &layer_state = state.layer_states[&layer];

        //TODO: post-processing fx support

        program_handle_t last_program = 0;
        texture_handle_t last_texture = 0;

        for (auto &bucket : layer_state.render_buckets) {
            auto &mat = bucket.second->material;
            auto program_info = state.linked_programs.find(&mat)->second;
            auto tex_handle = state.prepared_textures.find(&mat.pimpl->texture)->second;

            if (program_info.handle != last_program) {
                glUseProgram(program_info.handle);
                last_program = program_info.handle;

                auto view_mat_loc = program_info.view_matrix_uniform_loc;
                if (view_mat_loc != -1) {
                    glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, g_view_matrix);
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
    }

    void GLRenderer::render(Renderer &renderer, const TimeDelta delta) {
        auto &state = g_renderer_states[&renderer];

        _activate_gl_context(renderer.pimpl->window.pimpl->handle);

        if (renderer.pimpl->window.pimpl->dirty_resolution) {
            Vector2u res = renderer.pimpl->window.pimpl->properties.resolution;
            glViewport(0, 0, res.x, res.y);
            renderer.pimpl->window.pimpl->dirty_resolution = false;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _rebuild_scene(renderer);

        for (auto *layer : renderer.pimpl->render_layers) {
            _render_layer(renderer, *layer);
        }

        glfwSwapBuffers(renderer.pimpl->window.pimpl->handle);
    }
}
