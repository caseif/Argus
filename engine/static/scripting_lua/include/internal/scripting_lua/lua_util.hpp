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

#include "internal/scripting_lua/context_data.hpp"
#include "internal/scripting_lua/lua_language_plugin.hpp"
#include "internal/scripting_lua/managed_state.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#pragma GCC diagnostic pop

namespace argus {
    // forward declarations
    class LuaLanguagePlugin;

    lua_State *create_lua_state(LuaLanguagePlugin &plugin, LuaContextData &context_data);

    void destroy_lua_state(lua_State *state);

    LuaLanguagePlugin *get_plugin_from_state(lua_State *state);

    LuaContextData *get_context_data_from_state(lua_State *state);

    const std::shared_ptr<ManagedLuaState> &to_managed_state(lua_State *state);
}
