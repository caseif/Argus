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

#include "argus/scripting/script_context.hpp"
#include "internal/scripting/module_scripting.hpp"
#include "internal/scripting/pimpl/script_context.hpp"

#include <string>
#include <vector>

namespace argus {
    ScriptContext::ScriptContext(std::string language, void *plugin_data) {
        auto it = g_lang_plugins.find(language);
        if (it == g_lang_plugins.cend()) {
            Logger::default_logger().fatal("Unknown scripting language '%s'", language.c_str());
        }

        pimpl = new pimpl_ScriptContext(std::move(language), it->second, plugin_data);
    }

    ScriptContext::~ScriptContext(void) = default;

    void *ScriptContext::get_plugin_data_ptr(void) {
        return pimpl->plugin_data;
    }

    ObjectWrapper ScriptContext::invoke_script_function(const std::string &fn_name,
            const std::vector<ObjectWrapper> &params) {
        UNUSED(fn_name);
        UNUSED(params);

        //TODO
        return {};
    }
}
