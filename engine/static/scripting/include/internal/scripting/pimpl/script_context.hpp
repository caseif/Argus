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

#include "argus/scripting/scripting_language_plugin.hpp"

#include <string>

namespace argus {
    struct pimpl_ScriptContext {
        const std::string language;
        ScriptingLanguagePlugin *const plugin;
        void *const plugin_data;

        pimpl_ScriptContext(std::string language, ScriptingLanguagePlugin *plugin, void *plugin_data):
            language(std::move(language)),
            plugin(plugin),
            plugin_data(plugin_data) {
        }
    };
}
