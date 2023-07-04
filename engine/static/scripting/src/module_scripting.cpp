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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"

#include "argus/core/module.hpp"

#include "argus/scripting.hpp"
#include "internal/scripting/module_scripting.hpp"

#include <cstdio>

namespace argus {
    std::vector<ScriptingLanguagePlugin> g_lang_plugins;
    std::map<std::string, BoundTypeDef> g_bound_types;
    std::map<std::string, BoundFunctionDef> g_bound_global_fns;
    std::map<std::string, BoundFunctionDef> g_bound_member_fns;

    static void _bind_to_plugins(void) {
        for (auto &plugin : g_lang_plugins) {
            for (const auto &type : g_bound_types) {
                plugin.bind_type(type.second);
            }

            for (const auto &fn : g_bound_global_fns) {
                plugin.bind_global_function(fn.second);
            }
        }
    }

    void update_lifecycle_scripting(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PostInit: {
                _bind_to_plugins();
                break;
            }
            default:
                break;
        }
    }
}
