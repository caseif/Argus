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

#include "argus/scripting/registration.hpp"

#include "internal/scripting/angelscript_proxy.hpp"
#include "internal/scripting/module_scripting.hpp"

#include <string>

namespace argus {
    int register_global_function(const std::string &name, void *fn) {
        return g_as_script_engine->RegisterGlobalFunction(name.c_str(), asFUNCTION(fn), asCALL_STDCALL);
    }
}
