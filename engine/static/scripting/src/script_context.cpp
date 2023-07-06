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
#include "argus/lowlevel/vector.hpp"

#include "argus/core/engine.hpp"

#include "argus/scripting/script_context.hpp"
#include "internal/scripting/module_scripting.hpp"
#include "internal/scripting/pimpl/script_context.hpp"

#include <string>
#include <vector>

namespace argus {
    ScriptContext *g_script_context;

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

    void ScriptContext::load_script(const std::string &uid) {
        pimpl->plugin->load_script(*this, uid);
    }

    ObjectWrapper ScriptContext::invoke_script_function(const std::string &fn_name,
            const std::vector<ObjectWrapper> &params) {
        return pimpl->plugin->invoke_script_function(*this, fn_name, params);
    }

    ScriptContext &create_script_context(const std::string &language) {
        if (get_current_lifecycle_stage() != LifecycleStage::Init) {
            Logger::default_logger().fatal("Script contexts may only be created during Init stage");
        }

        auto plugin_it = g_lang_plugins.find(language);
        if (plugin_it == g_lang_plugins.cend()) {
            static char buf[1024];
            strcpy(buf, language.c_str());
            Logger::default_logger().fatal("No plugin is loaded for scripting language: %d", 5);
        }

        void *plugin_data = plugin_it->second->create_context_data();

        auto *context = new ScriptContext(language, plugin_data);

        g_script_contexts.push_back(context);

        return *context;
    }

    void destroy_script_context(ScriptContext &context) {
        remove_from_vector(g_script_contexts, &context);

        auto *plugin = context.pimpl->plugin;
        plugin->destroy_context_data(context.pimpl->plugin_data);

        delete &context;
    }
}
