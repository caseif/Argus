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

#include "argus/resman.hpp"

#include "internal/scripting_lua/lua_language_plugin.hpp"
#include "internal/scripting_lua/module_scripting_lua.hpp"
#include "internal/scripting_lua/loader/lua_script_loader.hpp"

namespace argus {
    static LuaScriptLoader *g_res_loader;
    static LuaLanguagePlugin *g_plugin;

    void update_lifecycle_scripting_lua(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PreInit: {
                g_plugin = new LuaLanguagePlugin();
                register_scripting_language(*g_plugin);
                break;
            }
            case LifecycleStage::Init: {
                g_res_loader = new LuaScriptLoader();
                ResourceManager::instance().register_loader(*g_res_loader);

                break;
            }
            case LifecycleStage::PostDeinit: {
                // the scripting module unloads script resources during the
                // Deinit stage, so we have to wait to delete the loader until
                // PostDeinit
                delete g_res_loader;

                //delete g_plugin;

                break;
            }
            default:
                break;
        }
    }
}
