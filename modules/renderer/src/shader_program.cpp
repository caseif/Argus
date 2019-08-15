// module core
#include "internal/util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/defines.hpp"
#include "internal/glext.hpp"

#define __LOG_MAX_LEN 255

namespace argus {

    using namespace glext;

    ShaderProgram::ShaderProgram(const std::vector<const Shader*> &shaders):
            shaders(shaders) {
        built = false;
    }

    static GLint _compile_shader(const GLenum type, std::string const &src) {
        GLint gl_shader = glCreateShader(type);

        const char *c_str = src.c_str();
        glShaderSource(gl_shader, 1, &c_str, nullptr);

        glCompileShader(gl_shader);

        int res;
        glGetShaderiv(gl_shader, GL_COMPILE_STATUS, &res);
        if (res == GL_FALSE) {
            char log[__GL_LOG_MAX_LEN + 1];
            glGetShaderInfoLog(gl_shader, __GL_LOG_MAX_LEN, nullptr, log);
            _ARGUS_FATAL("Failed to compile shader:\n%s\n", log);
        }

        return gl_shader;
    }

    void ShaderProgram::link(void) {
        // create the program, to start
        gl_program = glCreateProgram();

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
            attribute vec4 )" __ATTRIB_TEXCOORD R"(;

            uniform mat4 )" __UNIFORM_PROJECTION R"(;

            varying vec4 color;
            varying vec2 texCoord;

            vec2 position;

            // begin sub-shader concatenation
        )";

        // set up the fragment shader's globals
        std::stringstream bootstrap_frag_ss;
        bootstrap_frag_ss << R"(
            #version 110 core

            uniform sampler2DArray )" __UNIFORM_TEXTURE R"(;

            varying vec4 color;
            varying vec2 texCoord;

            // begin sub-shader concatenation
        )";

        // now we concatenate the source for each sub-shader
        for (const Shader *shader : shaders) {
            (shader->type == GL_VERTEX_SHADER ? bootstrap_vert_ss : bootstrap_frag_ss) << shader->src << "\n";
        }

        // we open the main function and add a little boilerplate
        bootstrap_vert_ss << R"(
            int main() {
                position = in_position;
                color = in_color;
                texCoord = in_texCoord;

        )";
        bootstrap_frag_ss << R"(
            int main() {
        )";

        // then we insert the calls to each sub-shaders entry point into the main() function
        //TODO: deal with priorities
        for (const Shader *shader : shaders) {
            (shader->type == GL_VERTEX_SHADER ? bootstrap_vert_ss : bootstrap_frag_ss) << "    " << shader->entry_point << "();\n";
        }

        // finally, we close bootstrap main() functions with a bit more boilerplate

        // for the vertex shader, we copy the position variable to the final output
        bootstrap_vert_ss << R"(
                gl_Position = projectionMatrix * vec4(position, 0.0, 1.0);
            }
        )";

        // for the fragment shader, we compute the final output color and copy it to the output
        bootstrap_frag_ss << R"(
                vec4 texColor = texture2D()" __UNIFORM_TEXTURE R"(, texCoord);
                gl_FragColor = color * texColor;
            }
        )";

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
            char log[__LOG_MAX_LEN + 1];
            glGetShaderInfoLog(gl_program, __LOG_MAX_LEN, nullptr, log);
            _ARGUS_FATAL("Failed to link program:\n%s\n", log);
        }

        // delete bootstrap shaders
        glDetachShader(gl_program, bootstrap_vert_handle);
        glDeleteShader(bootstrap_vert_handle);
        glDetachShader(gl_program, bootstrap_frag_handle);
        glDeleteShader(bootstrap_frag_handle);

        std::vector<std::string> bootstrap_uniforms {"texture"};
        for (std::string uniform_id : bootstrap_uniforms) {
            uniforms.insert({uniform_id, glGetUniformLocation(gl_program, uniform_id.c_str())});
        }

        for (const Shader *shader : shaders) {
            for (std::string uniform_id : shader->uniform_ids) {
                uniforms.insert({uniform_id, glGetUniformLocation(gl_program, uniform_id.c_str())});
            }
        }

        built = true;
    }

    //TODO: thread-safety
    void ShaderProgram::delete_program(void) {
        if (built) {
            glDeleteProgram(gl_program);
        }
    }

    void ShaderProgram::use_program(void) {
        if (!built) {
            link();
        }

        glUseProgram(gl_program);
    }

    GLuint ShaderProgram::get_handle(void) const {
        if (!built) {
            _ARGUS_FATAL("Attempted to get handle to shader program before it was built\n");
        }
        return gl_program;
    }

    GLint ShaderProgram::get_uniform_location(std::string const &uniform_id) const {
        auto it = uniforms.find(uniform_id);
        if (it == uniforms.end()) {
            _ARGUS_FATAL("Attempted to get non-existent shader uniform %s\n", uniform_id.c_str());
        }
        return it->second;
    }

}
