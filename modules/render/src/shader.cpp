/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/shader.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/glext.hpp"
#include "internal/render/pimpl/shader.hpp"

#include <initializer_list>
#include <string>

#define GEN_TRANSFORM_SHADER(entry, uniform) Shader::create_vertex_shader("\
        uniform mat4 " uniform ";       \n\
                                        \n\
        void " entry "() {              \n\
            position = (" uniform " * vec4(position, 0.0, 1.0)).xy;   \n\
        }                               \n\
    ", entry, 2147483647, {uniform})

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_Shader));

    using namespace glext;

    Shader g_layer_transform_shader = GEN_TRANSFORM_SHADER("_argus_apply_layer_transform", _UNIFORM_LAYER_TRANSFORM);

    Shader g_group_transform_shader = GEN_TRANSFORM_SHADER("_argus_apply_group_transform", _UNIFORM_GROUP_TRANSFORM);

    Shader::Shader(const unsigned int type, const std::string &src, const std::string &entry_point,
            const int priority, const std::initializer_list<std::string> &uniform_ids):
            pimpl(&g_pimpl_pool.construct<pimpl_Shader>(
                type,
                src,
                entry_point,
                priority,
                uniform_ids
            )) {
    }

    Shader Shader::create_vertex_shader(const std::string &src, const std::string &entry_point,
            const int priority, const std::initializer_list<std::string> &uniform_ids) {
        return Shader(SHADER_VERTEX, src, entry_point, priority, uniform_ids);
    }

    Shader Shader::create_fragment_shader(const std::string &src, const std::string &entry_point,
            const int priority, const std::initializer_list<std::string> &uniform_ids) {
        return Shader(SHADER_FRAGMENT, src, entry_point, priority, uniform_ids);
    }

}
