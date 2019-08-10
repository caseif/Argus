// module core
#include "internal/util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"

#define __LOG_MAX_LEN 255

namespace argus {

    using namespace glext;

    ShaderProgram::ShaderProgram(const std::initializer_list<Shader*> shaders):
            shaders(shaders) {
        built = false;
    }

    void ShaderProgram::link(void) {
        gl_program = glCreateProgram();

        // initialize bootstrap shaders
        std::stringstream bootstrap_vert_ss;
        std::stringstream bootstrap_frag_ss;
        bootstrap_vert_ss << "#version 110\n\nint main() {\n";
        bootstrap_frag_ss << "#version 110\n\nint main() {\n";

        //TODO: deal with priorities
        for (Shader *shader : shaders) {
            if (!shader->built) {
                shader->compile();
            }

            glAttachShader(gl_program, shader->gl_shader);

            // add call to this shader's entry point
            std::stringstream &ss = shader->type == GL_VERTEX_SHADER ? bootstrap_vert_ss : bootstrap_frag_ss;
            ss << "    " << shader->entry_point << "();\n";
        }

        // close main() functions
        bootstrap_vert_ss << "}\n";
        bootstrap_frag_ss << "}\n";

        Shader &bootstrap_vert = Shader::create_vertex_shader(bootstrap_vert_ss.str(), "", 0, {});
        Shader &bootstrap_frag = Shader::create_fragment_shader(bootstrap_frag_ss.str(), "", 0, {});

        bootstrap_vert.compile();
        glAttachShader(gl_program, bootstrap_vert.gl_shader);
        bootstrap_frag.compile();
        glAttachShader(gl_program, bootstrap_frag.gl_shader);

        glLinkProgram(gl_program);

        int res;
        glGetProgramiv(gl_program, GL_LINK_STATUS, &res);
        if (res == GL_FALSE) {
            char log[__LOG_MAX_LEN + 1];
            glGetShaderInfoLog(gl_program, __LOG_MAX_LEN, nullptr, log);
            _ARGUS_FATAL("Failed to link program:\n%s\n", log);
        }

        for (Shader *shader : shaders) {
            glDetachShader(gl_program, shader->gl_shader);

            for (std::string uniform_id : shader->uniform_ids) {
                uniforms.insert({uniform_id, glGetUniformLocation(gl_program, uniform_id.c_str())});
            }
        }

        // deinit bootstrap shaders
        glDetachShader(gl_program, bootstrap_vert.gl_shader);
        bootstrap_vert.destroy();
        glDetachShader(gl_program, bootstrap_frag.gl_shader);
        bootstrap_frag.destroy();

        built = true;
    }

    //TODO: thread-safety
    void ShaderProgram::destroy(void) {
        glDeleteProgram(gl_program);

        delete this;
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
