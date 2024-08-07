/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/memory.hpp"

#include "argus/core/engine.hpp"

#include "argus/render/common/shader.hpp"
#include "internal/render/pimpl/common/shader.hpp"

#include <string>
#include <type_traits>
#include <vector>

#include <cstdint>

namespace argus {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_Shader));

    Shader::Shader(const std::string &uid, const std::string &type, ShaderStage stage,
            const std::vector<uint8_t> &src):
        m_pimpl(&g_pimpl_pool.construct<pimpl_Shader>(uid, type, stage, src)) {
    }

    Shader::Shader(const Shader &rhs) noexcept:
        m_pimpl(&g_pimpl_pool.construct<pimpl_Shader>(*rhs.m_pimpl)) {
    }

    Shader::Shader(Shader &&rhs) noexcept:
        m_pimpl(rhs.m_pimpl) {
        rhs.m_pimpl = nullptr;
    }

    Shader::~Shader(void) {
        if (m_pimpl == nullptr) {
            return;
        }

        g_pimpl_pool.destroy(m_pimpl);
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
        return m_pimpl->uid;
    }

    const std::string &Shader::get_type(void) const {
        return m_pimpl->type;
    }

    ShaderStage Shader::get_stage(void) const {
        return m_pimpl->stage;
    }

    const std::vector<uint8_t> &Shader::get_source(void) const {
        return m_pimpl->src;
    }

    bool ShaderReflectionInfo::has_attr(const std::string &name) const {
        return attribute_locations.find(name) != attribute_locations.end();
    }

    std::optional<uint32_t> ShaderReflectionInfo::get_attr_loc(const std::string &name) const {
        auto it = attribute_locations.find(name);
        return it != attribute_locations.end()
                ? std::make_optional(it->second)
                : std::nullopt;
    }

    void ShaderReflectionInfo::get_attr_loc_and_then(const std::string &name, const std::function<void(uint32_t)>& fn) const {
        auto loc = get_attr_loc(name);
        if (loc.has_value()) {
            fn(loc.value());
        }
    }

    void ShaderReflectionInfo::set_attr_loc(const std::string &name, uint32_t loc) {
        attribute_locations[name] = loc;
    }

    bool ShaderReflectionInfo::has_output(const std::string &name) const {
        return output_locations.find(name) != output_locations.end();
    }

    std::optional<uint32_t> ShaderReflectionInfo::get_output_loc(const std::string &name) const {
        auto it = output_locations.find(name);
        return it != output_locations.end()
                ? std::make_optional(it->second)
                : std::nullopt;
    }

    void ShaderReflectionInfo::get_output_loc_and_then(const std::string &name,
            const std::function<void(uint32_t)>& fn) const {
        auto loc = get_output_loc(name);
        if (loc.has_value()) {
            fn(loc.value());
        }
    }

    void ShaderReflectionInfo::set_output_loc(const std::string &name, uint32_t loc) {
        output_locations[name] = loc;
    }

    bool ShaderReflectionInfo::has_uniform(const std::string &name) const {
        return uniform_variable_locations.find(name)
                != uniform_variable_locations.end();
    }

    bool ShaderReflectionInfo::has_uniform(const std::string &ubo, const std::string &name) const {
        auto ubo_inst_name = get_ubo_instance_name(ubo);
        if (!ubo_inst_name.has_value()) {
            return false;
        }

        auto joined_name = ubo_inst_name.value().get() + "." + name;
        auto it = uniform_variable_locations.find(joined_name);
        return it != uniform_variable_locations.end();
    }

    std::optional<uint32_t> ShaderReflectionInfo::get_uniform_loc(const std::string &name) const {
        auto it = uniform_variable_locations.find(name);
        return it != uniform_variable_locations.end()
                ? std::make_optional(it->second)
                : std::nullopt;
    }

    std::optional<uint32_t> ShaderReflectionInfo::get_uniform_loc(const std::string &ubo,
            const std::string &name) const {
        auto ubo_inst_name = get_ubo_instance_name(ubo);
        if (!ubo_inst_name.has_value()) {
            return std::nullopt;
        }

        auto joined_name = ubo_inst_name.value().get() + "." + name;
        return get_uniform_loc(joined_name);
    }

    void ShaderReflectionInfo::get_uniform_loc_and_then(const std::string &name,
            const std::function<void(uint32_t)>& fn) const {
        auto loc = get_uniform_loc(name);
        if (loc.has_value()) {
            fn(loc.value());
        }
    }

    void ShaderReflectionInfo::get_uniform_loc_and_then(const std::string &ubo, const std::string &name,
            const std::function<void(uint32_t)>& fn) const {
        auto loc = get_uniform_loc(ubo, name);
        if (loc.has_value()) {
            fn(loc.value());
        }
    }

    void ShaderReflectionInfo::set_uniform_loc(const std::string &name, uint32_t loc) {
        uniform_variable_locations[name] = loc;
    }

    void ShaderReflectionInfo::set_uniform_loc(const std::string &ubo, const std::string &name, uint32_t loc) {
        auto ubo_inst_name = get_ubo_instance_name(ubo);
        argus_assert(ubo_inst_name.has_value(), "Tried to set location for uniform variable in non-existent buffer");

        auto joined_name = ubo_inst_name.value().get() + "." + name;
        set_uniform_loc(joined_name, loc);
    }

    bool ShaderReflectionInfo::has_ubo(const std::string &name) const {
        return ubo_bindings.find(name) != ubo_bindings.cend();
    }

    std::optional<uint32_t> ShaderReflectionInfo::get_ubo_binding(const std::string &name) const {
        auto it = ubo_bindings.find(name);
        return it != ubo_bindings.end()
                ? std::make_optional(it->second)
                : std::nullopt;
    }

    void ShaderReflectionInfo::get_ubo_binding_and_then(const std::string &name,
            const std::function<void(uint32_t)>& fn) const {
        auto loc = get_ubo_binding(name);
        if (loc.has_value()) {
            fn(loc.value());
        }
    }

    void ShaderReflectionInfo::set_ubo_binding(const std::string &name, uint32_t loc) {
        ubo_bindings[name] = loc;
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const std::string>> ShaderReflectionInfo::get_ubo_instance_name(
            const std::string &name) const {
        auto it = ubo_instance_names.find(name);
        return it != ubo_instance_names.end()
                ? std::make_optional(std::reference_wrapper<const std::string>(it->second))
                : std::nullopt;
    }

    void ShaderReflectionInfo::set_ubo_instance_name(const std::string &ubo_name, const std::string &instance_name) {
        ubo_instance_names[ubo_name] = instance_name;
    }
}
