/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// module lowlevel
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/common/shader.hpp"
#include "internal/render/pimpl/common/shader.hpp"

#include <string>
#include <type_traits>

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

        g_pimpl_pool.destroy(pimpl);
    }

    ShaderStage operator |(ShaderStage lhs, ShaderStage rhs) {
        return static_cast<ShaderStage>(static_cast<std::underlying_type<ShaderStage>::type>(lhs) |
                                        static_cast<std::underlying_type<ShaderStage>::type>(rhs));
    }

    ShaderStage &operator |=(ShaderStage &lhs, ShaderStage rhs) {
        return lhs = static_cast<ShaderStage>(static_cast<std::underlying_type<ShaderStage>::type>(lhs) |
                                              static_cast<std::underlying_type<ShaderStage>::type>(rhs));
    }

    ShaderStage operator &(ShaderStage lhs, ShaderStage rhs) {
        return static_cast<ShaderStage>(static_cast<std::underlying_type<ShaderStage>::type>(lhs) &
                                        static_cast<std::underlying_type<ShaderStage>::type>(rhs));
    }

    ShaderStage Shader::get_stage(void) const {
        return pimpl->stage;
    }
}
