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

#include "argus/core/downstream_config.hpp"
#include "argus/core/engine.hpp"
#include "argus/core/module.hpp"

#include "internal/scripting/bind.hpp"
#include "internal/scripting/core_bindings.hpp"
#include "internal/scripting/handles.hpp"
#include "internal/scripting/module_scripting.hpp"
#include "internal/scripting/pimpl/script_context.hpp"

#include <string>
#include <typeindex>
#include <unordered_set>
#include <vector>

namespace argus {
    static constexpr const char *k_init_fn_name = "init";

    std::map<std::string, ScriptingLanguagePlugin *> g_lang_plugins;
    std::map<std::string, std::string> g_media_type_langs;
    std::map<std::string, BoundTypeDef> g_bound_types;
    std::map<std::type_index, std::string> g_bound_type_indices;
    std::map<std::string, BoundFunctionDef> g_bound_global_fns;
    std::map<std::string, BoundEnumDef> g_bound_enums;
    std::map<std::type_index, std::string> g_bound_enum_indices;
    std::vector<ScriptContext *> g_script_contexts;
    std::map<std::string, std::unordered_set<const Resource *>> g_loaded_resources;

    static Result<void, BindingError> _resolve_all_parameter_types(void) {
        for (auto &[_, type] : g_bound_types) {
            auto res = resolve_parameter_types(type);
            if (res.is_err()) {
                return res;
            }
        }

        for (auto &[_, fn] : g_bound_global_fns) {
            auto res = resolve_parameter_types(fn);
            if (res.is_err()) {
                return res;
            }
        }

        return ok<void, BindingError>();
    }

    static void _run_init_script(const std::string &uid) {
        try {
            auto context_res = load_script(uid);
            if (context_res.is_err()) {
                crash("Failed to run init script: " + context_res.unwrap_err().msg);
            }

            auto init_res = context_res.unwrap().invoke_script_function(k_init_fn_name, {});
            if (init_res.is_err()) {
                crash("Failed to run init script: " + std::string(init_res.unwrap_err().msg));
            }
        } catch (const std::exception &ex) {
            crash("Failed to run init script: " + std::string(ex.what()));
        }
    }

    extern "C" void update_lifecycle_scripting(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Init: {
                register_lowlevel_bindings();
                register_core_bindings();

                register_object_destroyed_performer();

                break;
            }
            case LifecycleStage::PostInit: {
                // parameter type resolution is deferred to ensure that all
                // types have been registered first
                _resolve_all_parameter_types();
                for (auto context : g_script_contexts) {
                    apply_bindings_to_context(*context).expect("Failed to apply bindings to script context");
                }

                const auto &scripting_params = get_scripting_parameters();
                if (scripting_params.main.has_value()) {
                    auto &uid = scripting_params.main.value();
                    // run it during the first iteration of the update loop
                    run_on_game_thread([&uid]() { _run_init_script(uid); });
                }

                break;
            }
            case LifecycleStage::Deinit: {
                for (auto context : g_script_contexts) {
                    auto &plugin = *context->m_pimpl->plugin;
                    auto *data = context->m_pimpl->plugin_data;
                    if (data != nullptr) {
                        plugin.destroy_context_data(data);
                    }
                }

                for (const auto &[_, plugin] : g_lang_plugins) {
                    delete plugin;
                }
                break;
            }
            default:
                break;
        }
    }
}
