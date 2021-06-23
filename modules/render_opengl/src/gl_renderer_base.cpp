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
#include "argus/lowlevel/memory.hpp"
#include "argus/lowlevel/threading.hpp"
#include "argus/lowlevel/time.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "internal/core/core_util.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "internal/wm/pimpl/window.hpp"

// module render
#include "argus/render/common/material.hpp"
#include "argus/render/common/render_layer.hpp"
#include "argus/render/common/renderer.hpp"
#include "argus/render/common/shader.hpp"
#include "argus/render/common/texture_data.hpp"
#include "argus/render/common/transform.hpp"
#include "internal/render/pimpl/common/material.hpp"
#include "internal/render/pimpl/common/renderer.hpp"
#include "internal/render/pimpl/common/shader.hpp"
#include "internal/render/pimpl/common/texture_data.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"
#include "internal/render/renderer_impl.hpp"

// module render_opengl
#include "internal/render_opengl/gl_renderer_base.hpp"
#include "internal/render_opengl/gl_renderer_2d.hpp"
#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/glext.hpp"
#include "internal/render_opengl/glfw_include.hpp"
#include "internal/render_opengl/globals.hpp"
#include "internal/render_opengl/layer_state.hpp"
#include "internal/render_opengl/processed_render_object.hpp"
#include "internal/render_opengl/render_bucket.hpp"
#include "internal/render_opengl/renderer_state.hpp"

#include <algorithm>
#include <atomic>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cstdio>
#include <cstring>

#define _FRAME_VERT_SHADER "\
    #version 330 core \n\
    in vec2 " SHADER_ATTRIB_IN_POSITION "; \n\
    in vec2 " SHADER_ATTRIB_IN_TEXCOORD "; \n\
    out vec2 " FRAME_SHADER_PASS_TEXCOORD "; \n\
    void main() { \n\
        gl_Position = vec4(" SHADER_ATTRIB_IN_POSITION ", 0.0, 1.0); \n\
        " FRAME_SHADER_PASS_TEXCOORD " = " SHADER_ATTRIB_IN_TEXCOORD "; \n\
    }"

#define _FRAME_FRAG_SHADER "\n\
    #version 330 core \n\
    in vec2 " FRAME_SHADER_PASS_TEXCOORD "; \n\
    out vec4 " SHADER_ATTRIB_OUT_FRAGDATA "; \n\
    uniform sampler2D screenTex; \n\
    void main() { \n\
        " SHADER_ATTRIB_OUT_FRAGDATA " = texture(screenTex, " FRAME_SHADER_PASS_TEXCOORD "); \n\
        //" SHADER_ATTRIB_OUT_FRAGDATA " = vec4(1.0, 0.0, 0.0, 1.0); \n\
    }"

namespace argus {
    // forward declarations
    class RenderLayer2D;

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

    static shader_handle_t _compile_shader(const ShaderStage stage, const char *src, const unsigned int src_len) {
        GLuint shader_stage;
        switch (stage) {
            case ShaderStage::VERTEX:
                shader_stage = GL_VERTEX_SHADER;
                break;
            case ShaderStage::FRAGMENT:
                shader_stage = GL_FRAGMENT_SHADER;
                break;
            default:
                _ARGUS_FATAL("Unrecognized shader stage ordinal %d\n", stage);
        }

        auto shader_handle = glCreateShader(shader_stage);
        if (!glIsShader(shader_handle)) {
            _ARGUS_FATAL("Failed to create shader: %d\n", glGetError());
        }

        int src_len_i = src_len;

        glShaderSource(shader_handle, 1, &src, &src_len_i);

        glCompileShader(shader_handle);

        int res;
        glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &res);
        if (res == GL_FALSE) {
            int log_len;
            glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &log_len);
            char *log = new char[log_len + 1];
            glGetShaderInfoLog(shader_handle, log_len, nullptr, log);
            std::string stage_str;
            switch (stage) {
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
            delete[] log;
        }

        return shader_handle;
    }

    // it is expected that the shaders will already be attached to the program when this function is called
    static void _link_program(program_handle_t program, VertexAttributes attrs) {
        unsigned int attrib_index = 0;
        if (attrs & VertexAttributes::POSITION) {
            glBindAttribLocation(program, SHADER_ATTRIB_LOC_POSITION, SHADER_ATTRIB_IN_POSITION);
        }
        if (attrs & VertexAttributes::NORMAL) {
            glBindAttribLocation(program, SHADER_ATTRIB_LOC_NORMAL, SHADER_ATTRIB_IN_NORMAL);
        }
        if (attrs & VertexAttributes::COLOR) {
            glBindAttribLocation(program, SHADER_ATTRIB_LOC_COLOR, SHADER_ATTRIB_IN_COLOR);
        }
        if (attrs & VertexAttributes::TEXCOORD) {
            glBindAttribLocation(program, SHADER_ATTRIB_LOC_TEXCOORD, SHADER_ATTRIB_IN_TEXCOORD);
        }

        glBindFragDataLocation(program, 0, SHADER_ATTRIB_OUT_FRAGDATA);

        glLinkProgram(program);

        int res;
        glGetProgramiv(program, GL_LINK_STATUS, &res);
        if (res == GL_FALSE) {
            int log_len;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
            char *log = new char[log_len];
            glGetProgramInfoLog(program, GL_INFO_LOG_LENGTH, nullptr, log);
            _ARGUS_FATAL("Failed to link program: %s\n", log);
            delete[] log;
        }
    }

    static void _build_shaders(RendererState &state, const Material &material) {
        auto existing_program_it = state.linked_programs.find(&material);
        if (existing_program_it != state.linked_programs.end()) {
            return;
        }

        auto program_handle = glCreateProgram();
        if (!glIsProgram(program_handle)) {
            _ARGUS_FATAL("Failed to create program: %d\n", glGetError());
        }

        for (auto *shader : material.pimpl->shaders) {
            shader_handle_t shader_handle;

            auto existing_shader_it = state.compiled_shaders.find(shader);
            if (existing_shader_it != state.compiled_shaders.end()) {
                shader_handle = existing_shader_it->second;
            } else {
                shader_handle = _compile_shader(shader->pimpl->stage, shader->pimpl->src, shader->pimpl->src_len);

                state.compiled_shaders.insert({ shader, shader_handle });
            }

            glAttachShader(program_handle, shader_handle);
        }

        _link_program(program_handle, material.pimpl->attributes);

        auto proj_mat_loc = glGetUniformLocation(program_handle, SHADER_UNIFORM_VIEW_MATRIX);

        state.linked_programs[&material] = { program_handle, proj_mat_loc };

        for (auto *shader : material.pimpl->shaders) {
            glDetachShader(program_handle, state.compiled_shaders[shader]);
        }
    }

    static void _prepare_texture(RendererState &state, const Material &material) {
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

    static void _rebuild_scene(RendererState &state) {
        for (auto *layer : state.renderer.pimpl->render_layers) {
            LayerState &layer_state = state.get_layer_state(*layer, true);

            auto &layer_transform = layer->get_transform();
            if (layer_transform.pimpl->dirty) {
                multiply_matrices(g_view_matrix, layer_transform.as_matrix(), layer_state.view_matrix);
                layer_transform.pimpl->dirty = false;
            }

            render_layer_2d(reinterpret_cast<RenderLayer2D&>(*layer), state,
                    reinterpret_cast<Layer2DState&>(layer_state));

            for (auto bucket_it : layer_state.render_buckets) {
                auto &mat = bucket_it.second->material;

                _build_shaders(state, mat);

                _prepare_texture(state, mat);
            }
        }
    }

    static void _draw_layer_to_framebuffer(LayerState &layer_state) {
        auto &layer = layer_state.layer;
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
            auto &mat = bucket.second->material;
            auto program_info = state.linked_programs.find(&mat)->second;
            auto tex_handle = state.prepared_textures.find(&mat.pimpl->texture)->second;

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

    static void _draw_framebuffer_to_screen(LayerState &layer_state) {
        auto &layer = layer_state.layer;
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

    static void _setup_framebuffer(RendererState &state) {
        state.frame_vert_shader = _compile_shader(ShaderStage::VERTEX, _FRAME_VERT_SHADER, sizeof(_FRAME_VERT_SHADER));
        state.frame_frag_shader = _compile_shader(ShaderStage::FRAGMENT, _FRAME_FRAG_SHADER, sizeof(_FRAME_FRAG_SHADER));

        state.frame_program = glCreateProgram();

        glAttachShader(state.frame_program, state.frame_vert_shader);
        glAttachShader(state.frame_program, state.frame_frag_shader);

        _link_program(state.frame_program, VertexAttributes::POSITION | VertexAttributes::TEXCOORD);

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

    void GLRenderer::init(Renderer &renderer) {
        _activate_gl_context(renderer.pimpl->window.pimpl->handle);

        glext::init_opengl_extensions();

        int gl_major;
        int gl_minor;
        const unsigned char *gl_version_str = glGetString(GL_VERSION);
        glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
        glGetIntegerv(GL_MINOR_VERSION, &gl_minor);
        if (gl_major < 3 || (gl_major == 3 && gl_minor < 3)) {
            _ARGUS_FATAL("Argus requires support for OpenGL 3.3 or higher (got %d.%d)\n", gl_major, gl_minor);
        }

        _ARGUS_INFO("Obtained OpenGL %d.%d context (%s)\n", gl_major, gl_minor, gl_version_str);

        const GLubyte *ver_str = glGetString(GL_VERSION);

        renderer_states.insert({ &renderer, RendererState(renderer) });

        //TODO: actually do something
        if (glDebugMessageCallbackARB != nullptr) {
            glDebugMessageCallbackARB(_gl_debug_callback, nullptr);
        }

        _setup_framebuffer(get_renderer_state(renderer));
    }

    void GLRenderer::deinit(Renderer &renderer) {
        get_renderer_state(renderer).~RendererState();
    }

    void GLRenderer::deinit_texture(const TextureData &texture) {
        for (auto &state : renderer_states) {
            auto &textures = state.second.prepared_textures;
            auto existing_it = textures.find(&texture);
            if (existing_it != textures.end()) {
                glDeleteTextures(1, &existing_it->second);
                textures.erase(existing_it);
            }
        }
    }

    void GLRenderer::deinit_shader(const Shader &shader) {
        for (auto &state : renderer_states) {
            auto &shaders = state.second.compiled_shaders;
            auto existing_it = shaders.find(&shader);
            if (existing_it != shaders.end()) {
                glDeleteShader(existing_it->second);
                shaders.erase(existing_it);
            }
        }
    }

    void GLRenderer::deinit_material(const Material &material) {
        for (auto &state : renderer_states) {
            for (auto *layer_state : state.second.all_layer_states) {
                auto &buckets = layer_state->render_buckets;
                auto bucket_it = buckets.find(&material);
                if (bucket_it != buckets.end()) {
                    try_delete_buffer(bucket_it->second->vertex_array);
                    try_delete_buffer(bucket_it->second->vertex_buffer);
                    bucket_it->second->~RenderBucket();
                    buckets.erase(bucket_it);
                }
            }

            auto &programs = state.second.linked_programs;
            auto program_it = programs.find(&material);
            if (program_it != programs.end()) {
                glDeleteProgram(program_it->second.handle);
            }

            programs.erase(program_it);
        }
    }

    void GLRenderer::render(Renderer &renderer, const TimeDelta delta) {
        auto &state = get_renderer_state(renderer);

        _activate_gl_context(renderer.pimpl->window.pimpl->handle);

        _rebuild_scene(state);

        // set up state for drawing scene to framebuffers
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDisable(GL_CULL_FACE);

        for (auto *layer : renderer.pimpl->render_layers) {
            auto &layer_state = state.get_layer_state(*layer);
            _draw_layer_to_framebuffer(layer_state);
        }

        // set up state for drawing framebuffers to screen

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        for (auto *layer : renderer.pimpl->render_layers) {
            auto &layer_state = state.get_layer_state(*layer);

            _draw_framebuffer_to_screen(layer_state);
        }

        glfwSwapBuffers(renderer.pimpl->window.pimpl->handle);
    }

    RendererState &GLRenderer::get_renderer_state(Renderer &renderer) {
        auto it = renderer_states.find(&renderer);
        _ARGUS_ASSERT(it != renderer_states.cend(), "Cannot find renderer state");
        return it->second;
    }
}