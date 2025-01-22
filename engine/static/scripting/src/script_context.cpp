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
#include "argus/scripting/manager.hpp"
#include "internal/scripting/module_scripting.hpp"
#include "internal/scripting/pimpl/script_context.hpp"

#include <string>
#include <vector>

#include "../../scripting_lua/include/internal/scripting_lua/defines.hpp"

namespace argus {
    ScriptContext::ScriptContext(std::string language, void *plugin_data) {
        auto plugin_opt = ScriptManager::instance().get_language_plugin(language);
        if (!plugin_opt.has_value()) {
            crash("Unknown scripting language '%s'", language.c_str());
        }

        m_pimpl = new pimpl_ScriptContext(std::move(language), &plugin_opt.value().get(), plugin_data);
    }

    ScriptContext::~ScriptContext(void) = default;

    void *ScriptContext::get_plugin_data_ptr(void) {
        return m_pimpl->plugin_data;
    }

    Result<void, ScriptLoadError> ScriptContext::load_script(const std::string &uid) {
        return ScriptManager::instance().load_resource(k_plugin_lang_name, uid)
                .and_then<void>([this](const auto &res) { return this->load_script(res); });
    }

    Result<void, ScriptLoadError> ScriptContext::load_script(const Resource &resource) {
        auto &mgr = ScriptManager::instance();
        auto &plugin = mgr.get_language_plugin(m_pimpl->language).value().get();
        auto &media_types = plugin.get_media_types();
        if (std::find(media_types.cbegin(), media_types.cend(), resource.prototype.media_type) ==
                media_types.cend()) {
            return err<void, ScriptLoadError>(resource.prototype.uid, "Resource with media type '" + resource.prototype.media_type
                    + "' cannot be loaded by plugin '" + m_pimpl->language + "'");
        }

        mgr.move_resource(plugin.get_language_name(), resource);
        return m_pimpl->plugin->load_script(*this, resource);
    }

    Result<ObjectWrapper, ScriptInvocationError> ScriptContext::invoke_script_function(const std::string &fn_name,
            const std::vector<ObjectWrapper *> &params) {
        return m_pimpl->plugin->invoke_script_function(*this, fn_name, params);
    }

    ScriptContext &create_script_context(const std::string &language) {
        auto &mgr = ScriptManager::instance();

        auto plugin_opt = mgr.get_language_plugin(language);
        if (!plugin_opt.has_value()) {
            crash("No plugin is loaded for scripting language: %s", language.c_str());
        }

        void *plugin_data = plugin_opt.value().get().create_context_data();

        auto &context = *new ScriptContext(language, plugin_data);

        mgr.register_context(context);

        if (get_current_lifecycle_stage() >= LifecycleStage::PostInit) {
            mgr.apply_bindings_to_context(context).expect("Failed to apply bindings to script context");
        }

        return context;
    }

    void destroy_script_context(ScriptContext &context) {
        auto &mgr = ScriptManager::instance();

        mgr.unregister_context(context);

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

        auto &mgr = ScriptManager::instance();

        auto media_type = res.unwrap().prototype.media_type;
        auto plugin_opt = mgr.get_media_type_plugin(media_type);
        if (!plugin_opt.has_value()) {
            return err<ScriptContext &, ScriptLoadError>(uid, "No plugin registered for media type '"
                    + res.unwrap().prototype.media_type + "'");
        }

        auto &context = create_script_context(plugin_opt.value().get().get_language_name());

        auto load_res = context.load_script(res.unwrap());
        if (load_res.is_err()) {
            return err<ScriptContext &, ScriptLoadError>(load_res.unwrap_err());
        }

        return ok<ScriptContext &, ScriptLoadError>(context);
    }
}
