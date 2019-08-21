// module core
#include "internal/util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/defines.hpp"
#include "internal/glext.hpp"

#define GEN_TRANSFORM_SHADER(entry, uniform) Shader::create_vertex_shader_stack("\
        uniform mat4 " uniform ";       \n\
                                        \n\
        void " entry "() {              \n\
            position = (" uniform " * vec4(position, 0.0, 1.0)).xy;   \n\
        }                               \n\
    ", entry, {uniform})

namespace argus {

    using namespace glext;

    Shader g_layer_transform_shader = GEN_TRANSFORM_SHADER("_argus_apply_layer_transform", __UNIFORM_LAYER_TRANSFORM);

    Shader g_group_transform_shader = GEN_TRANSFORM_SHADER("_argus_apply_group_transform", __UNIFORM_GROUP_TRANSFORM);

    Shader::Shader(const unsigned int type, std::string const &src, std::string const &entry_point,
            std::initializer_list<std::string> const &uniform_ids):
            type(type),
            src(src),
            entry_point(entry_point),
            uniform_ids(std::vector<std::string>(uniform_ids)) {
    }
            
    Shader &Shader::create_vertex_shader(const std::string src, const std::string entry_point,
            std::initializer_list<std::string> const &uniform_ids) {
        return *new Shader(SHADER_VERTEX, src, entry_point, uniform_ids);
    }
    
    Shader Shader::create_vertex_shader_stack(const std::string src, const std::string entry_point,
            std::initializer_list<std::string> const &uniform_ids) {
        return Shader(SHADER_VERTEX, src, entry_point, uniform_ids);
    }

    Shader &Shader::create_fragment_shader(const std::string src, const std::string entry_point,
            std::initializer_list<std::string> const &uniform_ids) {
        return *new Shader(SHADER_FRAGMENT, src, entry_point, uniform_ids);
    }
    
    Shader Shader::create_fragment_shader_stack(const std::string src, const std::string entry_point,
            std::initializer_list<std::string> const &uniform_ids) {
        return Shader(SHADER_FRAGMENT, src, entry_point, uniform_ids);
    }

}
