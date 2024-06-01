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

#include "internal/scripting_angelscript/module_scripting_angelscript.hpp"
#include "internal/scripting_angelscript/script_handle.hpp"

#include <string>

namespace argus {
    ScriptHandle::ScriptHandle(void) noexcept = default;

    void ScriptHandle::ExecuteFunction(const std::string &name) const {
        asIScriptFunction *fn;

        auto it = fn_ptrs.find(name);
        if (it != fn_ptrs.cend()) {
            fn = it->second;
        } else {
            fn = mod->GetFunctionByName(name.c_str());
        }

        auto *ctx = g_as_script_engine->CreateContext();
        ctx->Prepare(fn);
        ctx->Execute();
    }
}
