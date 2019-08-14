// module core
#include "internal/util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"

#define GEN_TRANSFORM_SHADER(entry, uniform) Shader::create_vertex_shader_stack("    \
        uniform mat4 " uniform ";       \
                                        \
        void " entry "() {              \
            position = position * transform;   \
        }                               \
    ", entry, {"projection", uniform})

namespace argus {

    using namespace glext;

    Shader g_layer_transform_shader = GEN_TRANSFORM_SHADER("_argus_apply_layer_transform", "_argus_layer_transform");

    Shader g_group_transform_shader = GEN_TRANSFORM_SHADER("_argus_apply_group_transform", "_argus_group_transform");

    Shader::Shader(const GLenum type, std::string const &src, std::string const &entry_point,
            const std::initializer_list<std::string> uniform_ids):
            type(type),
            src(src),
            entry_point(entry_point),
            uniform_ids(uniform_ids) {
    }
            
    Shader &Shader::create_vertex_shader(const std::string src, const std::string entry_point,
            const std::initializer_list<std::string> uniform_ids) {
        return *new Shader(GL_VERTEX_SHADER, src, entry_point, uniform_ids);
    }
    
    Shader Shader::create_vertex_shader_stack(const std::string src, const std::string entry_point,
            const std::initializer_list<std::string> uniform_ids) {
        return Shader(GL_VERTEX_SHADER, src, entry_point, uniform_ids);
    }

    Shader &Shader::create_fragment_shader(const std::string src, const std::string entry_point,
            const std::initializer_list<std::string> uniform_ids) {
        return *new Shader(GL_FRAGMENT_SHADER, src, entry_point, uniform_ids);
    }
    
    Shader Shader::create_fragment_shader_stack(const std::string src, const std::string entry_point,
            const std::initializer_list<std::string> uniform_ids) {
        return Shader(GL_FRAGMENT_SHADER, src, entry_point, uniform_ids);
    }

}
