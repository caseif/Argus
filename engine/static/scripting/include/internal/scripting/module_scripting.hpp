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

#pragma once

#include "argus/core/module.hpp"

#include "argus/scripting/bridge.hpp"
#include "argus/scripting/scripting_language_plugin.hpp"
#include "internal/scripting/angelscript_proxy.hpp"

#include <vector>

namespace argus {
    extern std::vector<ScriptingLanguagePlugin> g_lang_plugins;
    extern std::map<std::string, BoundTypeDef> g_registered_types;
    extern std::map<std::string, BoundFunctionDef> g_registered_fns;

    extern asIScriptEngine *g_as_script_engine;

    void update_lifecycle_scripting(LifecycleStage stage);
}
