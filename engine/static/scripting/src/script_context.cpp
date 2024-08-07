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
#include "argus/lowlevel/result.hpp"

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
            crash("Unknown scripting language '%s'", language.c_str());
        }

        m_pimpl = new pimpl_ScriptContext(std::move(language), it->second, plugin_data);
    }

    ScriptContext::~ScriptContext(void) = default;

    void *ScriptContext::get_plugin_data_ptr(void) {
        return m_pimpl->plugin_data;
    }

    Result<void, ScriptLoadError> ScriptContext::load_script(const std::string &uid) {
        return m_pimpl->plugin->load_resource(uid)
                .and_then<void>([this](const auto &res) { return this->load_script(res); });
    }

    Result<void, ScriptLoadError> ScriptContext::load_script(const Resource &resource) {
        auto lang_it = g_media_type_langs.find(resource.prototype.media_type);
        if (lang_it == g_media_type_langs.cend() || lang_it->second != m_pimpl->language) {
            return err<void, ScriptLoadError>(resource.prototype.uid, "Resource with media type '" + resource.prototype.media_type
                    + "' cannot be loaded by plugin '" + m_pimpl->language + "'");
        }

        m_pimpl->plugin->move_resource(resource);
        return m_pimpl->plugin->load_script(*this, resource);
    }

    Result<ObjectWrapper, ScriptInvocationError> ScriptContext::invoke_script_function(const std::string &fn_name,
            const std::vector<ObjectWrapper> &params) {
        return m_pimpl->plugin->invoke_script_function(*this, fn_name, params);
    }

    ScriptContext &create_script_context(const std::string &language) {
        auto plugin_it = g_lang_plugins.find(language);
        if (plugin_it == g_lang_plugins.cend()) {
            crash("No plugin is loaded for scripting language: %s", language.c_str());
        }

        void *plugin_data = plugin_it->second->create_context_data();

        auto *context = new ScriptContext(language, plugin_data);

        g_script_contexts.push_back(context);

        if (get_current_lifecycle_stage() >= LifecycleStage::PostInit) {
            apply_bindings_to_context(*context).expect("Failed to apply bindings to script context");
        }

        return *context;
    }

    void destroy_script_context(ScriptContext &context) {
        remove_from_vector(g_script_contexts, &context);

        auto *plugin = context.m_pimpl->plugin;
        plugin->destroy_context_data(context.m_pimpl->plugin_data);

        delete &context;
    }

    Result<ScriptContext &, ScriptLoadError> load_script(const std::string &uid) {
        auto res = ResourceManager::instance().get_resource(uid);

        if (!res.is_ok()) {
            const auto &res_err = res.unwrap_err();
            return err<ScriptContext &, ScriptLoadError>(uid, "Resource load failed (return code "
                    + std::to_string(static_cast<std::underlying_type_t<ResourceErrorReason>>(res_err.reason))
                    + (!res_err.info.empty() ? (": " + res_err.info) : "") + ")");
        }

        auto lang_it = g_media_type_langs.find(res.unwrap().prototype.media_type);
        if (lang_it == g_media_type_langs.cend()) {
            return err<ScriptContext &, ScriptLoadError>(uid, "No plugin registered for media type '"
                    + res.unwrap().prototype.media_type + "'");
        }

        auto &context = create_script_context(lang_it->second);

        auto load_res = context.load_script(res.unwrap());
        if (load_res.is_err()) {
            return err<ScriptContext &, ScriptLoadError>(load_res.unwrap_err());
        }

        return ok<ScriptContext &, ScriptLoadError>(context);
    }
}
