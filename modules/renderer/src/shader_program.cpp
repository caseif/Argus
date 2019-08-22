// module lowlevel
#include "internal/logging.hpp"

// module core
#include "internal/core_util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/glext.hpp"

#include <sstream>
#include <SDL2/SDL_opengl.h>

#define __LOG_MAX_LEN 255

namespace argus {

    using namespace glext;

    // this is transposed from the actual matrix, since GL interprets it in column-major order
    float g_ortho_matrix[16] = {
        2.0,  0.0,  0.0,  0.0,
        0.0, -2.0,  0.0,  0.0,
        0.0,  0.0,  1.0,  0.0,
       -1.0,  1.0,  1.0,  1.0
    };

    ShaderProgram::ShaderProgram(const std::vector<const Shader*> &shaders):
            shaders(shaders),
            initialized(false),
            needs_rebuild(true) {
    }

    void ShaderProgram::update_shaders(std::vector<const Shader*> &shaders) {
        this->shaders = shaders;
        needs_rebuild = true;
    }

    static GLint _compile_shader(const GLenum type, std::string const &src) {
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
        GLuint gl_program = static_cast<GLuint>(program_handle);

        if (initialized) {
            glDeleteProgram(gl_program);
        }

        // create the program, to start
        gl_program = glCreateProgram();
        program_handle = static_cast<handle_t>(gl_program);

        if (!initialized) {
            initialized = true;
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
            in vec2 )" __ATTRIB_POSITION R"(;
            in vec4 )" __ATTRIB_COLOR R"(;
            in vec3 )" __ATTRIB_TEXCOORD R"(;

            uniform mat4 )" __UNIFORM_PROJECTION R"(;

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
            uniform sampler2DArray )" __UNIFORM_TEXTURE R"(;

            in vec4 color;
            in vec3 texCoord;

            out vec4 )" __OUT_FRAGDATA R"(;

            // begin sub-shader concatenation)";

        // now we concatenate the source for each sub-shader
        for (const Shader *shader : shaders) {
            switch (shader->type) {
                case SHADER_VERTEX:
                    bootstrap_vert_ss << shader->src << "\n";
                    break;
                case SHADER_FRAGMENT:
                    bootstrap_frag_ss << shader->src << "\n";
                    break;
                default:
                    _ARGUS_FATAL("Unrecognized shader type ID %d\n", shader->type);
            }
        }

        // we open the main function and add a little boilerplate
        bootstrap_vert_ss << R"(
            // end sub-shader concatenation

            void main() {
                position = ()" __UNIFORM_PROJECTION R"( * vec4()" __ATTRIB_POSITION R"(, 0.0, 1.0)).xy;
                color = )" __ATTRIB_COLOR R"(;
                texCoord = )" __ATTRIB_TEXCOORD R"(;

                // begin sub-shader invocation)";

        bootstrap_frag_ss << R"(
            // end sub-shader concatenation

            void main() {
                // begin sub-shader invocation)";

        // then we insert the calls to each sub-shaders entry point into the main() function
        //TODO: deal with priorities
        for (const Shader *shader : shaders) {
            switch (shader->type) {
                case SHADER_VERTEX:
                    bootstrap_vert_ss << "\n    " << shader->entry_point << "();";
                    break;
                case SHADER_FRAGMENT:
                    bootstrap_frag_ss << "\n    " << shader->entry_point << "();";
                    break;
                default:
                    _ARGUS_FATAL("Unrecognized shader type ID %d\n", shader->type);
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

                vec4 texel = texture()" __UNIFORM_TEXTURE R"(, texCoord);
                )" __OUT_FRAGDATA R"( = texel + color;
            }
        )";

        //_ARGUS_DEBUG("Bootstrap vertex shader source:\n----------\n%s\n----------\n", bootstrap_vert_ss.str().c_str());
        //_ARGUS_DEBUG("Bootstrap fragment shader source:\n----------\n%s\n----------\n", bootstrap_frag_ss.str().c_str());

        // compile each bootstrap shader

        GLint bootstrap_vert_handle = _compile_shader(GL_VERTEX_SHADER, bootstrap_vert_ss.str());
        glAttachShader(program_handle, bootstrap_vert_handle);

        GLint bootstrap_frag_handle = _compile_shader(GL_FRAGMENT_SHADER, bootstrap_frag_ss.str());
        glAttachShader(program_handle, bootstrap_frag_handle);

        glBindAttribLocation(program_handle, __ATTRIB_LOC_POSITION, __ATTRIB_POSITION);
        glBindAttribLocation(program_handle, __ATTRIB_LOC_COLOR, __ATTRIB_COLOR);
        glBindAttribLocation(program_handle, __ATTRIB_LOC_TEXCOORD, __ATTRIB_TEXCOORD);

        glBindFragDataLocation(program_handle, 0, __OUT_FRAGDATA);

        glLinkProgram(program_handle);

        int res;
        glGetProgramiv(program_handle, GL_LINK_STATUS, &res);
        if (res == GL_FALSE) {
            int log_len;
            glGetShaderiv(program_handle, GL_INFO_LOG_LENGTH, &log_len);
            char *log = new char[log_len];
            glGetShaderInfoLog(program_handle, __LOG_MAX_LEN, nullptr, log);
            _ARGUS_FATAL("Failed to link program:\n%s\n", log);
            delete[] log;
        }

        // delete bootstrap shaders
        glDetachShader(program_handle, bootstrap_vert_handle);
        glDeleteShader(bootstrap_vert_handle);
        glDetachShader(program_handle, bootstrap_frag_handle);
        glDeleteShader(bootstrap_frag_handle);

        std::vector<std::string> bootstrap_uniforms {__UNIFORM_PROJECTION, __UNIFORM_TEXTURE};
        for (std::string uniform_id : bootstrap_uniforms) {
            GLint uniform_loc = glGetUniformLocation(program_handle, uniform_id.c_str());
            uniforms.insert({uniform_id, uniform_loc});
        }

        for (const Shader *shader : shaders) {
            for (std::string const &uniform_id : shader->uniform_ids) {
                GLint uniform_loc = glGetUniformLocation(program_handle, uniform_id.c_str());
                uniforms.insert({uniform_id, uniform_loc});
            }
        }

        glUseProgram(program_handle);
        glUniformMatrix4fv(get_uniform_location(__UNIFORM_PROJECTION), 1, GL_FALSE, g_ortho_matrix);
        glUseProgram(0);

        needs_rebuild = false;
    }

    handle_t ShaderProgram::get_uniform_location(std::string const &uniform_id) const {
        auto it = uniforms.find(uniform_id);
        if (it == uniforms.end()) {
            _ARGUS_FATAL("Attempted to get non-existent shader uniform %s\n", uniform_id.c_str());
        }
        return it->second;
    }

}