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
#include "internal/render/module_render.hpp"
#include "internal/render/renderer_impl.hpp"
#include "internal/render/pimpl/shader.hpp"

#include <initializer_list>
#include <string>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_Shader));

    Shader::Shader(const ShaderStage stage, const char *const src, const size_t src_len):
            pimpl(&g_pimpl_pool.construct<pimpl_Shader>(
                stage,
                src,
                src_len
            )) {
    }

    Shader::~Shader(void) {
        get_renderer_impl().deinit_shader(*this);
    }

    Shader Shader::create_shader(const ShaderStage stage, const char *const src, const size_t src_len) {
        return Shader(stage, src, src_len);
    }

}
