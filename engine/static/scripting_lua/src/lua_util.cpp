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

#include "internal/scripting_lua/lua_util.hpp"

namespace argus {
    lua_State *create_lua_state(void) {
        auto *state = luaL_newstate();
        if (state == nullptr) {
            Logger::default_logger().fatal("Failed to create Lua state");
        }

        luaL_openlibs(state);

        return state;
    }

    void destroy_lua_state(lua_State *state) {
        lua_close(state);
    }
}
