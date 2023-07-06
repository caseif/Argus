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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"

#include "argus/core/module.hpp"

#include "argus/scripting.hpp"
#include "internal/scripting/module_scripting.hpp"
#include "internal/scripting/pimpl/script_context.hpp"

#include <cstdio>

namespace argus {
    std::map<std::string, ScriptingLanguagePlugin *> g_lang_plugins;
    std::map<std::string, BoundTypeDef> g_bound_types;
    std::map<std::string, BoundFunctionDef> g_bound_global_fns;
    std::vector<ScriptContext*> g_script_contexts;
    std::map<std::string, std::vector<Resource*>> g_loaded_resources;

    static void _bind_to_plugins(void) {
        for (auto *context : g_script_contexts) {
            for (const auto &type : g_bound_types) {
                context->pimpl->plugin->bind_type(*context, type.second);
            }

            for (const auto &fn : g_bound_global_fns) {
                context->pimpl->plugin->bind_global_function(*context, fn.second);
            }
        }
    }

    // temporary testing stuff

    struct Adder {
        int i = 0;
        int increment() {
            i += 1;
            return i;
        }

        int add(int j) {
            i += j;
            return j;
        }
    };

    static Adder *_create_adder(int i) {
        return new Adder { i };
    }

    void update_lifecycle_scripting(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Init: {
                bind_global_function("create_adder", _create_adder);

                auto adder_type = create_type_def<Adder>("Adder");
                add_member_instance_function(adder_type, "increment", &Adder::increment);
                add_member_instance_function(adder_type, "add", &Adder::add);

                g_script_context = &create_script_context("lua");

                break;
            }
            case LifecycleStage::PostInit: {
                _bind_to_plugins();

                break;
            }
            default:
                break;
        }
    }
}
