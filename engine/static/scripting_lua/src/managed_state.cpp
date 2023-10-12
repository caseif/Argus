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

#include "internal/scripting_lua/context_data.hpp"
#include "internal/scripting_lua/lua_language_plugin.hpp"
#include "internal/scripting_lua/lua_util.hpp"
#include "internal/scripting_lua/managed_state.hpp"

namespace argus {
    ManagedLuaState::ManagedLuaState(LuaLanguagePlugin &plugin, LuaContextData &context_data) :
        m_handle(create_lua_state(plugin, context_data)) {
    }

    ManagedLuaState::ManagedLuaState(ManagedLuaState &&rhs) noexcept : m_handle(rhs.m_handle) {
        rhs.m_handle = nullptr;
    }

    ManagedLuaState::~ManagedLuaState() {
        if (m_handle != nullptr) {
            destroy_lua_state(m_handle);
        }
    }

    ManagedLuaState::operator lua_State *(void) const {
        return m_handle;
    }

    lua_State *ManagedLuaState::get_handle(void) const {
        return m_handle;
    }
}
