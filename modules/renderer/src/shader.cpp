// module lowlevel
#include "internal/logging.hpp"

// module core
#include "internal/core_util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/glext.hpp"

#define GEN_TRANSFORM_SHADER(entry, uniform) Shader::create_vertex_shader_stack("\
        uniform mat4 " uniform ";       \n\
                                        \n\
        void " entry "() {              \n\
            position = (" uniform " * vec4(position, 0.0, 1.0)).xy;   \n\
        }                               \n\
    ", entry, 2147483647, {uniform})

namespace argus {

    using namespace glext;

    Shader g_layer_transform_shader = GEN_TRANSFORM_SHADER("_argus_apply_layer_transform", _UNIFORM_LAYER_TRANSFORM);

    Shader g_group_transform_shader = GEN_TRANSFORM_SHADER("_argus_apply_group_transform", _UNIFORM_GROUP_TRANSFORM);

    Shader::Shader(const unsigned int type, std::string const &src, std::string const &entry_point,
            const int priority, std::initializer_list<std::string> const &uniform_ids):
            type(type),
            src(src),
            entry_point(entry_point),
            priority(priority),
            uniform_ids(std::vector<std::string>(uniform_ids)) {
    }
            
    Shader &Shader::create_vertex_shader(const std::string src, const std::string entry_point,
            const int priority, std::initializer_list<std::string> const &uniform_ids) {
        return *new Shader(SHADER_VERTEX, src, entry_point, priority, uniform_ids);
    }

    Shader Shader::create_vertex_shader_stack(const std::string src, const std::string entry_point,
            const int priority, std::initializer_list<std::string> const &uniform_ids) {
        return Shader(SHADER_VERTEX, src, entry_point, priority, uniform_ids);
    }

    Shader &Shader::create_fragment_shader(const std::string src, const std::string entry_point,
            const int priority, std::initializer_list<std::string> const &uniform_ids) {
        return *new Shader(SHADER_FRAGMENT, src, entry_point, priority, uniform_ids);
    }

    Shader Shader::create_fragment_shader_stack(const std::string src, const std::string entry_point,
            const int priority, std::initializer_list<std::string> const &uniform_ids) {
        return Shader(SHADER_FRAGMENT, src, entry_point, priority, uniform_ids);
    }

}
