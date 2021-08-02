/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/common/shader.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/renderer_impl.hpp"
#include "internal/render/pimpl/common/shader.hpp"

#include <cstddef>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_Shader));

    Shader::Shader(ShaderStage stage, const std::string &src):
            pimpl(&g_pimpl_pool.construct<pimpl_Shader>(
                stage,
                src
            )) {
    }

    Shader::Shader(const Shader &rhs) noexcept:
        pimpl(&g_pimpl_pool.construct<pimpl_Shader>(*rhs.pimpl)) {
    }

    Shader::Shader(Shader &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Shader::~Shader(void) {
        if (pimpl == nullptr) {
            return;
        }

        get_renderer_impl().deinit_shader(*this);

        g_pimpl_pool.free(pimpl);
    }

    constexpr ShaderStage operator |(ShaderStage lhs, ShaderStage rhs) {
        return static_cast<ShaderStage>(static_cast<std::underlying_type<ShaderStage>::type>(lhs) |
                                        static_cast<std::underlying_type<ShaderStage>::type>(rhs));
    }

    ShaderStage &operator |=(ShaderStage &lhs, ShaderStage rhs) {
        return lhs = static_cast<ShaderStage>(static_cast<std::underlying_type<ShaderStage>::type>(lhs) |
                                              static_cast<std::underlying_type<ShaderStage>::type>(rhs));
    }

    constexpr ShaderStage operator &(ShaderStage lhs, ShaderStage rhs) {
        return static_cast<ShaderStage>(static_cast<std::underlying_type<ShaderStage>::type>(lhs) &
                                        static_cast<std::underlying_type<ShaderStage>::type>(rhs));
    }

    ShaderStage Shader::get_stage(void) const {
        return pimpl->stage;
    }
}
