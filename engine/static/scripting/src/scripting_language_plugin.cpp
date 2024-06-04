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

#include "argus/lowlevel/collections.hpp"

#include "argus/resman.hpp"

#include "argus/scripting/exception.hpp"
#include "argus/scripting/scripting_language_plugin.hpp"
#include "internal/scripting/module_scripting.hpp"

namespace argus {
    ScriptingLanguagePlugin::ScriptingLanguagePlugin(std::string lang_name,
            const std::initializer_list<std::string> &media_types):
        m_lang_name(std::move(lang_name)),
        m_media_types(media_types) {
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

        g_loaded_resources.find(get_language_name())->second.insert(res);

        return *res;
    }

    void ScriptingLanguagePlugin::move_resource(const Resource &resource) {
        g_loaded_resources.find(get_language_name())->second.insert(&resource);
    }

    void ScriptingLanguagePlugin::release_resource(const Resource &resource) {
        resource.release();
        g_loaded_resources.find(get_language_name())->second.erase(&resource);
    }

    void register_scripting_language(ScriptingLanguagePlugin &plugin) {
        g_lang_plugins.insert({ plugin.get_language_name(), &plugin });
        for (const auto &mt : plugin.get_media_types()) {
            auto existing = g_media_type_langs.find(mt);
            if (existing != g_media_type_langs.cend()) {
                Logger::default_logger().fatal("Media type '%s' is already associated with language plugin '%s'",
                        mt.c_str(), existing->second.c_str());
            }
            g_media_type_langs.insert({ mt, plugin.get_language_name() });
        }
        g_loaded_resources.insert({ plugin.get_language_name(), {} });
    }
}
