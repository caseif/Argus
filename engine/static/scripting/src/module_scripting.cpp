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

#include "argus/core/module.hpp"

#include "argus/scripting.hpp"
#include "internal/scripting/bind.hpp"
#include "internal/scripting/module_scripting.hpp"
#include "internal/scripting/pimpl/script_context.hpp"

#include <string>
#include <typeindex>
#include <unordered_set>
#include <vector>

namespace argus {
    std::map<std::string, ScriptingLanguagePlugin *> g_lang_plugins;
    std::map<std::string, std::string> g_media_type_langs;
    std::map<std::string, BoundTypeDef> g_bound_types;
    std::map<std::type_index, std::string> g_bound_type_indices;
    std::map<std::string, BoundFunctionDef> g_bound_global_fns;
    std::map<std::string, BoundEnumDef> g_bound_enums;
    std::map<std::type_index, std::string> g_bound_enum_indices;
    std::vector<ScriptContext*> g_script_contexts;
    std::map<std::string, std::unordered_set<const Resource*>> g_loaded_resources;

    static void _resolve_all_parameter_types(void) {
        for (auto &type : g_bound_types) {
            resolve_parameter_types(type.second);
        }

        for (auto &fn : g_bound_global_fns) {
            resolve_parameter_types(fn.second);
        }
    }

    std::vector<std::function<void(int)>> callbacks;

    static void _register_callback(std::function<void(int)> callback) {
        callbacks.push_back(callback);
    }

    static void _invoke_callbacks(int i) {
        for (const auto &callback : callbacks) {
            callback(i);
        }
    }

    void update_lifecycle_scripting(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Init: {
                bind_global_function("register_callback", _register_callback);
                bind_global_function("invoke_callbacks", _invoke_callbacks);

                break;
            }
            case LifecycleStage::PostInit: {
                // parameter type resolution is deferred to ensure that all
                // types have been registered first
                _resolve_all_parameter_types();
                for (auto context : g_script_contexts) {
                    apply_bindings_to_context(*context);
                }

                break;
            }
            case LifecycleStage::Deinit: {
                for (auto context : g_script_contexts) {
                    auto &plugin = *context->pimpl->plugin;
                    auto *data = context->pimpl->plugin_data;
                    if (data != nullptr) {
                        plugin.destroy_context_data(data);
                    }
                }

                for (const auto &plugin : g_lang_plugins) {
                    delete plugin.second;
                }
                break;
            }
            default:
                break;
        }
    }
}
