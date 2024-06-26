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

#pragma once

#include "internal/scripting_lua/defines.hpp"
#include "internal/scripting_lua/lua_language_plugin.hpp"

struct lua_State;

namespace argus {
    struct LuaContextData;

    class ManagedLuaState {
      private:
        lua_State *m_handle;

      public:
        ManagedLuaState(LuaLanguagePlugin &plugin, LuaContextData &context_data);

        ManagedLuaState(const ManagedLuaState &) = delete;

        ManagedLuaState(ManagedLuaState &&rhs) noexcept;

        ~ManagedLuaState(void);

        operator lua_State *(void) const;

        [[nodiscard]] lua_State *get_handle(void) const;
    };
}
