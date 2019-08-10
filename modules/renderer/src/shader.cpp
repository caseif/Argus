// module core
#include "internal/util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"

#define __SHADER_VERT 0
#define __SHADER_FRAG 1

namespace argus {

    using namespace glext;

    Shader::Shader(const GLenum type, std::string const &src, std::string const &entry_point, const int priority,
            const std::initializer_list<std::string> uniform_ids):
            type(type),
            src(src),
            entry_point(entry_point),
            priority(priority),
            uniform_ids(uniform_ids) {
        built = false;
    }

    void Shader::compile(void) {
        gl_shader = glCreateShader(type);

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
    }
            
    Shader &Shader::create_vertex_shader(const std::string src, const std::string entry_point,
            const int priority, const std::initializer_list<std::string> uniform_ids) {
        return *new Shader(GL_VERTEX_SHADER, src, entry_point, priority, uniform_ids);
    }

    Shader &Shader::create_fragment_shader(const std::string src, const std::string entry_point,
            const int priority, const std::initializer_list<std::string> uniform_ids) {
        return *new Shader(GL_FRAGMENT_SHADER, src, entry_point, priority, uniform_ids);
    }

    //TODO: thread-safety
    void Shader::destroy(void) {
        glDeleteShader(gl_shader);

        delete this;
    }

    GLuint Shader::get_handle(void) const {
        if (!built) {
            _ARGUS_FATAL("Attempted to get handle to shader before it was built\n");
        }
        return gl_shader;
    }

}
