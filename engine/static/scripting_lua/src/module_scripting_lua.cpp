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

#include "argus/core/module.hpp"

#include "internal/scripting_lua/lua_language_plugin.hpp"
#include "internal/scripting_lua/module_scripting_lua.hpp"

namespace argus {
    std::vector<lua_State*> g_lua_states;

    static LuaLanguagePlugin *plugin;

    void update_lifecycle_scripting_lua(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PreInit: {
                plugin = new LuaLanguagePlugin();
                register_scripting_language(plugin);
                break;
            }
            case LifecycleStage::Init: {
                auto *global_state = create_lua_state();
                g_lua_states.push_back(global_state);

                break;
            }
            case LifecycleStage::Deinit: {
                for (auto *state : g_lua_states) {
                    destroy_lua_state(state);
                }

                break;
            }
            default:
                break;
        }
    }
}
