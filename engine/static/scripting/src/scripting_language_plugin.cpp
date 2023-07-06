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

#include "argus/lowlevel/vector.hpp"

#include "argus/resman.hpp"

#include "argus/scripting/exception.hpp"
#include "argus/scripting/scripting_language_plugin.hpp"
#include "internal/scripting/module_scripting.hpp"

namespace argus {
    ScriptingLanguagePlugin::ScriptingLanguagePlugin(std::string lang_name) : lang_name(std::move(lang_name)) {
        g_loaded_resources.insert({ this->lang_name, {} });
    }

    ScriptingLanguagePlugin::~ScriptingLanguagePlugin(void) {
        for (auto *res : g_loaded_resources.find(get_language_name())->second) {
            res->release();
        }
    }

    Resource &ScriptingLanguagePlugin::load_resource(const std::string &uid) {
        Resource *res;
        try {
            res = &ResourceManager::instance().get_resource(uid);
        } catch (const ResourceNotPresentException &) {
            throw ScriptLoadException(uid, "Cannot load script (resource does not exist)");
        }

        g_loaded_resources.find(get_language_name())->second.push_back(res);

        return *res;
    }

    void ScriptingLanguagePlugin::release_resource(Resource &resource) {
        resource.release();
        remove_from_vector(g_loaded_resources.find(get_language_name())->second, &resource);
    }

    void register_scripting_language(ScriptingLanguagePlugin *plugin) {
        g_lang_plugins.insert({ plugin->get_language_name(), plugin });
    }
}
