/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/memory.hpp"
#include "internal/lowlevel/logging.hpp"

// module renderer
#include "argus/renderer/shader.hpp"
#include "argus/renderer/shader_program.hpp"
#include "internal/renderer/defines.hpp"
#include "internal/renderer/glext.hpp"
#include "internal/renderer/pimpl/shader.hpp"
#include "internal/renderer/pimpl/shader_program.hpp"

#include <SDL2/SDL_opengl.h>

#include <algorithm>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define _LOG_MAX_LEN 255

namespace argus {

    using namespace glext;

    static AllocPool g_pimpl_pool(sizeof(pimpl_ShaderProgram), 512);

    // this is transposed from the actual matrix, since GL interprets it in column-major order
    float g_ortho_matrix[16] = {
        2.0,  0.0,  0.0,  0.0,
        0.0, -2.0,  0.0,  0.0,
        0.0,  0.0,  1.0,  0.0,
       -1.0,  1.0,  1.0,  1.0
    };

    ShaderProgram::ShaderProgram(const std::vector<const Shader*> &shaders):
            pimpl(&g_pimpl_pool.construct<pimpl_ShaderProgram>()) {
        pimpl->shaders = decltype(pimpl->shaders)(shaders.begin(), shaders.end(), [](auto a, auto b){
            if (a->pimpl->priority != b->pimpl->priority) {
                return a->pimpl->priority > b->pimpl->priority;
            } else {
                return a->pimpl->entry_point.compare(b->pimpl->entry_point) < 0;
            }
        });
        pimpl->initialized = false;
        pimpl->needs_rebuild = true;
    }

    void ShaderProgram::update_shaders(const std::vector<const Shader*> &shaders) {
        pimpl->shaders.clear();
        std::copy(shaders.cbegin(), shaders.cend(), std::inserter(pimpl->shaders, pimpl->shaders.end()));
        pimpl->needs_rebuild = true;
    }

    static GLint _compile_shader(const GLenum type, const std::string &src) {
        GLint gl_shader = glCreateShader(type);

        if (!glIsShader(gl_shader)) {
            _ARGUS_FATAL("Failed to create %s shader\n", type == GL_VERTEX_SHADER ? "vertex" : "fragment");
        }

        const char *c_str = src.c_str();
        glShaderSource(gl_shader, 1, &c_str, nullptr);

        glCompileShader(gl_shader);

        int res;
        glGetShaderiv(gl_shader, GL_COMPILE_STATUS, &res);
        if (res == GL_FALSE) {
            int log_len;
            glGetShaderiv(gl_shader, GL_INFO_LOG_LENGTH, &log_len);
            char *log = new char[log_len];
            glGetShaderInfoLog(gl_shader, log_len, nullptr, log);
            _ARGUS_FATAL("Failed to compile %s shader:\n%s\n", type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
            delete[] log;
        }

        return gl_shader;
    }

    void ShaderProgram::link(void) {
        GLuint gl_program = static_cast<GLuint>(pimpl->program_handle);

        if (pimpl->initialized) {
            glDeleteProgram(gl_program);
        }

        // create the program, to start
        gl_program = glCreateProgram();
        pimpl->program_handle = static_cast<handle_t>(gl_program);

        if (!pimpl->initialized) {
            pimpl->initialized = true;
        }

        // We use a bit of a hack to enable multiple shaders while maintaining GLES support
        // Basically, each shader has its source concatenated to a monolithic "boostrap" shader.
        // Then, a call to each shader's entry point is inserted into the bootstrap shader's main() function.
        // There are two such bootstrap shaders, one for vertex shading and one for fragment shading.
        // The bootstrap shaders also set up some globals that are accessible by the "sub-shaders."

        // set up the vertex shader's globals
        std::stringstream bootstrap_vert_ss;
        bootstrap_vert_ss <<
            #ifdef USE_GLES
            R"(#version 300 es)"
            #else
            R"(#version 330 core)"
            #endif
            R"(
            in vec2 )" _ATTRIB_POSITION R"(;
            in vec4 )" _ATTRIB_COLOR R"(;
            in vec3 )" _ATTRIB_TEXCOORD R"(;

            uniform mat4 )" _UNIFORM_PROJECTION R"(;

            out vec4 color;
            out vec3 texCoord;

            vec2 position;

            // begin sub-shader concatenation
        )";

        // set up the fragment shader's globals
        std::stringstream bootstrap_frag_ss;
        bootstrap_frag_ss <<
            #ifdef USE_GLES
            R"(#version 300 es)"
            #else
            R"(#version 330 core)"
            #endif
            R"(
            precision mediump float;
            precision mediump int;
            precision mediump sampler2DArray;

            uniform sampler2DArray )" _UNIFORM_TEXTURE R"(;

            in vec4 color;
            in vec3 texCoord;

            out vec4 )" _OUT_FRAGDATA R"(;

            // begin sub-shader concatenation)";

        // now we concatenate the source for each sub-shader
        for (const Shader *const shader : pimpl->shaders) {
            switch (shader->pimpl->type) {
                case SHADER_VERTEX:
                    bootstrap_vert_ss << shader->pimpl->src << "\n";
                    break;
                case SHADER_FRAGMENT:
                    bootstrap_frag_ss << shader->pimpl->src << "\n";
                    break;
                default:
                    _ARGUS_FATAL("Unrecognized shader type ID %d\n", shader->pimpl->type);
            }
        }

        // we open the main function and add a little boilerplate
        bootstrap_vert_ss << R"(
            // end sub-shader concatenation

            void main() {
                position = ()" _UNIFORM_PROJECTION R"( * vec4()" _ATTRIB_POSITION R"(, 0.0, 1.0)).xy;
                color = )" _ATTRIB_COLOR R"(;
                texCoord = )" _ATTRIB_TEXCOORD R"(;

                // begin sub-shader invocation)";

        bootstrap_frag_ss << R"(
            // end sub-shader concatenation

            void main() {
                // begin sub-shader invocation)";

        // then we insert the calls to each sub-shaders entry point into the main() function
        for (const Shader *const shader : pimpl->shaders) {
            switch (shader->pimpl->type) {
                case SHADER_VERTEX:
                    bootstrap_vert_ss << "\n    " << shader->pimpl->entry_point << "();";
                    break;
                case SHADER_FRAGMENT:
                    bootstrap_frag_ss << "\n    " << shader->pimpl->entry_point << "();";
                    break;
                default:
                    _ARGUS_FATAL("Unrecognized shader type ID %d\n", shader->pimpl->type);
            }
        }

        // finally, we close bootstrap main() functions with a bit more boilerplate

        // for the vertex shader, we copy the position variable to the final output
        bootstrap_vert_ss << R"(
                // end sub-shader invocation

                gl_Position = vec4(position, 0.0, 1.0);
            }
        )";

        // for the fragment shader, we compute the final output color and copy it to the output
        bootstrap_frag_ss << R"(
                // end sub-shader invocation

                vec4 texel = texture()" _UNIFORM_TEXTURE R"(, texCoord);
                )" _OUT_FRAGDATA R"( = vec4((texel.rgb + color.rgb), texel.a * color.a);
            }
        )";

        //_ARGUS_DEBUG("Bootstrap vertex shader source:\n----------\n%s\n----------\n", bootstrap_vert_ss.str().c_str());
        //_ARGUS_DEBUG("Bootstrap fragment shader source:\n----------\n%s\n----------\n", bootstrap_frag_ss.str().c_str());

        // compile each bootstrap shader

        GLint bootstrap_vert_handle = _compile_shader(GL_VERTEX_SHADER, bootstrap_vert_ss.str());
        glAttachShader(pimpl->program_handle, bootstrap_vert_handle);

        GLint bootstrap_frag_handle = _compile_shader(GL_FRAGMENT_SHADER, bootstrap_frag_ss.str());
        glAttachShader(pimpl->program_handle, bootstrap_frag_handle);

        glBindAttribLocation(pimpl->program_handle, _ATTRIB_LOC_POSITION, _ATTRIB_POSITION);
        glBindAttribLocation(pimpl->program_handle, _ATTRIB_LOC_COLOR, _ATTRIB_COLOR);
        glBindAttribLocation(pimpl->program_handle, _ATTRIB_LOC_TEXCOORD, _ATTRIB_TEXCOORD);

        glBindFragDataLocation(pimpl->program_handle, 0, _OUT_FRAGDATA);

        glLinkProgram(pimpl->program_handle);

        int res;
        glGetProgramiv(pimpl->program_handle, GL_LINK_STATUS, &res);
        if (res == GL_FALSE) {
            int log_len;
            glGetShaderiv(pimpl->program_handle, GL_INFO_LOG_LENGTH, &log_len);
            char *log = new char[log_len];
            glGetShaderInfoLog(pimpl->program_handle, _LOG_MAX_LEN, nullptr, log);
            _ARGUS_FATAL("Failed to link program:\n%s\n", log);
            delete[] log;
        }

        // delete bootstrap shaders
        glDetachShader(pimpl->program_handle, bootstrap_vert_handle);
        glDeleteShader(bootstrap_vert_handle);
        glDetachShader(pimpl->program_handle, bootstrap_frag_handle);
        glDeleteShader(bootstrap_frag_handle);

        std::vector<std::string> bootstrap_uniforms {_UNIFORM_PROJECTION, _UNIFORM_TEXTURE};
        for (std::string uniform_id : bootstrap_uniforms) {
            GLint uniform_loc = glGetUniformLocation(pimpl->program_handle, uniform_id.c_str());
            pimpl->uniforms.insert({uniform_id, uniform_loc});
        }

        for (const Shader *shader : pimpl->shaders) {
            for (const std::string &uniform_id : shader->pimpl->uniform_ids) {
                GLint uniform_loc = glGetUniformLocation(pimpl->program_handle, uniform_id.c_str());
                pimpl->uniforms.insert({uniform_id, uniform_loc});
            }
        }

        glUseProgram(pimpl->program_handle);
        glUniformMatrix4fv(get_uniform_location(_UNIFORM_PROJECTION), 1, GL_FALSE, g_ortho_matrix);
        glUseProgram(0);

        pimpl->needs_rebuild = false;
    }

    void ShaderProgram::delete_program(void) {
        _ARGUS_ASSERT(pimpl->initialized, "Cannot delete uninitialized program.");
        glDeleteProgram(pimpl->program_handle);
        pimpl->initialized = false;
    }

    uniform_location_t ShaderProgram::get_uniform_location(const std::string &uniform_id) const {
        auto it = pimpl->uniforms.find(uniform_id);
        if (it == pimpl->uniforms.end()) {
            _ARGUS_FATAL("Attempted to get non-existent shader uniform %s\n", uniform_id.c_str());
        }
        return it->second;
    }

}
