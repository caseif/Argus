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
#include "argus/lowlevel/collections.hpp"

#include "argus/core/engine.hpp"

#include "argus/resman.hpp"

#include "argus/scripting/script_context.hpp"
#include "internal/scripting/bind.hpp"
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

        m_pimpl = new pimpl_ScriptContext(std::move(language), it->second, plugin_data);
    }

    ScriptContext::~ScriptContext(void) = default;

    void *ScriptContext::get_plugin_data_ptr(void) {
        return m_pimpl->plugin_data;
    }

    void ScriptContext::load_script(const std::string &uid) {
        auto &res = m_pimpl->plugin->load_resource(uid);

        this->load_script(res);
    }

    void ScriptContext::load_script(const Resource &resource) {
        auto lang_it = g_media_type_langs.find(resource.media_type);
        if (lang_it == g_media_type_langs.cend() || lang_it->second != m_pimpl->language) {
            throw ScriptLoadException(resource.uid, "Resource with media type '" + resource.prototype.media_type
                    + "' cannot be loaded by plugin '" + m_pimpl->language + "'");
        }

        m_pimpl->plugin->move_resource(resource);
        m_pimpl->plugin->load_script(*this, resource);
    }

    ObjectWrapper ScriptContext::invoke_script_function(const std::string &fn_name,
            const std::vector<ObjectWrapper> &params) {
        return m_pimpl->plugin->invoke_script_function(*this, fn_name, params);
    }

    ScriptContext &create_script_context(const std::string &language) {
        auto plugin_it = g_lang_plugins.find(language);
        if (plugin_it == g_lang_plugins.cend()) {
            Logger::default_logger().fatal("No plugin is loaded for scripting language: %s", language.c_str());
        }

        void *plugin_data = plugin_it->second->create_context_data();

        auto *context = new ScriptContext(language, plugin_data);

        g_script_contexts.push_back(context);

        if (get_current_lifecycle_stage() >= LifecycleStage::PostInit) {
            apply_bindings_to_context(*context);
        }

        return *context;
    }

    void destroy_script_context(ScriptContext &context) {
        remove_from_vector(g_script_contexts, &context);

        auto *plugin = context.m_pimpl->plugin;
        plugin->destroy_context_data(context.m_pimpl->plugin_data);

        delete &context;
    }

    ScriptContext &load_script(const std::string &uid) {
        try {
            auto &res = ResourceManager::instance().get_resource(uid);

            auto lang_it = g_media_type_langs.find(res.media_type);
            if (lang_it == g_media_type_langs.cend()) {
                throw ScriptLoadException(uid, "No plugin registered for media type '"
                        + res.prototype.media_type + "'");
            }

            auto &context = create_script_context(lang_it->second);

            context.load_script(res);

            return context;
        } catch (const ScriptLoadException &ex) {
            throw ex;
        } catch (const std::exception &ex) {
            throw ScriptLoadException(uid, ex.what());
        }
    }
}
