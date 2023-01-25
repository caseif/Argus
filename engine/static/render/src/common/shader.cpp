/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/memory.hpp"

#include "argus/render/common/shader.hpp"
#include "internal/render/pimpl/common/shader.hpp"

#include <string>
#include <type_traits>
#include <vector>

#include <cstdint>

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_Shader));

    Shader::Shader(const std::string &uid, const std::string &type, ShaderStage stage, const std::vector<uint8_t> &src)
            :
            pimpl(&g_pimpl_pool.construct<pimpl_Shader>(
                    uid,
                    type,
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

    ShaderStage operator|(ShaderStage lhs, ShaderStage rhs) {
        return ShaderStage(static_cast<std::underlying_type<ShaderStage>::type>(lhs) |
                           static_cast<std::underlying_type<ShaderStage>::type>(rhs));
    }

    ShaderStage &operator|=(ShaderStage &lhs, ShaderStage rhs) {
        return lhs = ShaderStage(static_cast<std::underlying_type<ShaderStage>::type>(lhs) |
                                 static_cast<std::underlying_type<ShaderStage>::type>(rhs));
    }

    ShaderStage operator&(ShaderStage lhs, ShaderStage rhs) {
        return ShaderStage(static_cast<std::underlying_type<ShaderStage>::type>(lhs) &
                           static_cast<std::underlying_type<ShaderStage>::type>(rhs));
    }

    const std::string &Shader::get_uid(void) const {
        return pimpl->uid;
    }

    const std::string &Shader::get_type(void) const {
        return pimpl->type;
    }

    ShaderStage Shader::get_stage(void) const {
        return pimpl->stage;
    }

    const std::vector<uint8_t> &Shader::get_source(void) const {
        return pimpl->src;
    }
}
