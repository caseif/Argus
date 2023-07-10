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

#include "argus/lowlevel/logging.hpp"

#include "internal/scripting_lua/defines.hpp"
#include "internal/scripting_lua/lua_language_plugin.hpp"
#include "internal/scripting_lua/lua_util.hpp"
#include "internal/scripting_lua/module_scripting_lua.hpp"

namespace argus {
    lua_State *create_lua_state(LuaLanguagePlugin &plugin) {
        auto *state = luaL_newstate();
        if (state == nullptr) {
            Logger::default_logger().fatal("Failed to create Lua state");
        }

        luaL_openlibs(state);

        lua_pushlightuserdata(state, &plugin);
        lua_setfield(state, LUA_REGISTRYINDEX, REG_KEY_PLUGIN_PTR);

        return state;
    }

    void destroy_lua_state(lua_State *state) {
        lua_close(state);
    }

    LuaLanguagePlugin *get_plugin_from_state(lua_State *state) {
        lua_getfield(state, LUA_REGISTRYINDEX, REG_KEY_PLUGIN_PTR);
        return reinterpret_cast<LuaLanguagePlugin *>(lua_touserdata(state, -1));
    }
}
