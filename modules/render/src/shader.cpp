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
#include "internal/render/pimpl/shader.hpp"

#include <initializer_list>
#include <string>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_Shader));

    Shader::Shader(const unsigned int type, const char *const src, const size_t src_len, const std::string &entry_point,
            const int order, const std::initializer_list<std::string> &uniform_ids):
            pimpl(&g_pimpl_pool.construct<pimpl_Shader>(
                type,
                src,
                entry_point,
                order,
                uniform_ids
            )) {
    }

    Shader Shader::create_vertex_shader(const char *const src, const size_t src_len, const std::string &entry_point,
            const int order, const std::initializer_list<std::string> &uniform_ids) {
        return Shader(SHADER_VERTEX, src, src_len, entry_point, order, uniform_ids);
    }

    Shader Shader::create_fragment_shader(const char *const src, const size_t src_len, const std::string &entry_point,
            const int order, const std::initializer_list<std::string> &uniform_ids) {
        return Shader(SHADER_FRAGMENT, src, src_len, entry_point, order, uniform_ids);
    }

}
