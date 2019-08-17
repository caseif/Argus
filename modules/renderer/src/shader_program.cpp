// module core
#include "internal/util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/defines.hpp"
#include "internal/glext.hpp"

#define __LOG_MAX_LEN 255

namespace argus {

    using namespace glext;

    using vmml::mat4f;

    // not sure why this needs to be row-major form...
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
            char log[log_len];
            glGetShaderInfoLog(gl_shader, log_len, nullptr, log);
            _ARGUS_FATAL("Failed to compile %s shader:\n%s\n", type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
        }

        return gl_shader;
    }

    void ShaderProgram::link(void) {
        if (initialized) {
            glDeleteProgram(gl_program);
        }

        // create the program, to start
        gl_program = glCreateProgram();

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
            attribute vec2 )" __ATTRIB_POSITION R"(;
            attribute vec4 )" __ATTRIB_COLOR R"(;
            attribute vec3 )" __ATTRIB_TEXCOORD R"(;

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

            // begin sub-shader concatenation)";

        // now we concatenate the source for each sub-shader
        for (const Shader *shader : shaders) {
            (shader->type == GL_VERTEX_SHADER ? bootstrap_vert_ss : bootstrap_frag_ss) << shader->src << "\n";
        }

        // we open the main function and add a little boilerplate
        bootstrap_vert_ss << R"(
            // end sub-shader concatenation

            void main() {
                position = ()" __UNIFORM_PROJECTION R"( * vec4(in_position, 0.0, 1.0)).xy;
                color = in_color;
                texCoord = in_texCoord;

                // begin sub-shader invocation)";

        bootstrap_frag_ss << R"(
            // end sub-shader concatenation

            void main() {
                // begin sub-shader invocation)";

        // then we insert the calls to each sub-shaders entry point into the main() function
        //TODO: deal with priorities
        for (const Shader *shader : shaders) {
            (shader->type == GL_VERTEX_SHADER ? bootstrap_vert_ss : bootstrap_frag_ss) << "\n    " << shader->entry_point << "();";
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
                gl_FragColor = texel + color;
            }
        )";

        //_ARGUS_DEBUG("Bootstrap vertex shader source:\n----------\n%s\n----------\n", bootstrap_vert_ss.str().c_str());
        //_ARGUS_DEBUG("Bootstrap fragment shader source:\n----------\n%s\n----------\n", bootstrap_frag_ss.str().c_str());

        // compile each bootstrap shader

        GLint bootstrap_vert_handle = _compile_shader(GL_VERTEX_SHADER, bootstrap_vert_ss.str());
        glAttachShader(gl_program, bootstrap_vert_handle);

        GLint bootstrap_frag_handle = _compile_shader(GL_FRAGMENT_SHADER, bootstrap_frag_ss.str());
        glAttachShader(gl_program, bootstrap_frag_handle);

        glBindAttribLocation(gl_program, __ATTRIB_LOC_POSITION, __ATTRIB_POSITION);
        glBindAttribLocation(gl_program, __ATTRIB_LOC_COLOR, __ATTRIB_COLOR);
        glBindAttribLocation(gl_program, __ATTRIB_LOC_TEXCOORD, __ATTRIB_TEXCOORD);

        glLinkProgram(gl_program);

        int res;
        glGetProgramiv(gl_program, GL_LINK_STATUS, &res);
        if (res == GL_FALSE) {
            int log_len;
            glGetShaderiv(gl_program, GL_INFO_LOG_LENGTH, &log_len);
            char log[log_len];
            glGetShaderInfoLog(gl_program, __LOG_MAX_LEN, nullptr, log);
            _ARGUS_FATAL("Failed to link program:\n%s\n", log);
        }

        // delete bootstrap shaders
        glDetachShader(gl_program, bootstrap_vert_handle);
        glDeleteShader(bootstrap_vert_handle);
        glDetachShader(gl_program, bootstrap_frag_handle);
        glDeleteShader(bootstrap_frag_handle);

        std::vector<std::string> bootstrap_uniforms {__UNIFORM_PROJECTION, __UNIFORM_TEXTURE};
        for (std::string uniform_id : bootstrap_uniforms) {
            GLint uniform_loc = glGetUniformLocation(gl_program, uniform_id.c_str());
            uniforms.insert({uniform_id, uniform_loc});
        }

        for (const Shader *shader : shaders) {
            for (std::string const &uniform_id : shader->uniform_ids) {
                GLint uniform_loc = glGetUniformLocation(gl_program, uniform_id.c_str());
                uniforms.insert({uniform_id, uniform_loc});
            }
        }

        glUseProgram(gl_program);
        glUniformMatrix4fv(get_uniform_location(__UNIFORM_PROJECTION), 1, GL_FALSE, g_ortho_matrix);
        glUseProgram(0);

        needs_rebuild = false;
    }

    GLint ShaderProgram::get_uniform_location(std::string const &uniform_id) const {
        auto it = uniforms.find(uniform_id);
        if (it == uniforms.end()) {
            _ARGUS_FATAL("Attempted to get non-existent shader uniform %s\n", uniform_id.c_str());
        }
        return it->second;
    }

}
