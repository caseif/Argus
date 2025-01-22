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

#include "internal/scripting/core_bindings.hpp"
#include "internal/scripting/handles.hpp"
#include "internal/scripting/module_scripting.hpp"

#include <string>
#include <vector>

namespace argus {
    static constexpr const char *k_init_fn_name = "init";

    static void _run_init_script(const std::string &uid) {
        auto context_res = load_script(uid);
        if (context_res.is_err()) {
            crash("Failed to run init script: " + context_res.unwrap_err().msg);
        }

        auto init_res = context_res.unwrap().invoke_script_function(k_init_fn_name, {});
        if (init_res.is_err()) {
            crash("Failed to run init script: " + std::string(init_res.unwrap_err().msg));
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
                ScriptManager::instance().resolve_types()
                        .expect("Failed to resolve parameter types");

                    ScriptManager::instance().apply_bindings_to_all_contexts()
                            .expect("Failed to apply bindings to script contexts");

                const auto &scripting_params = get_scripting_parameters();
                if (scripting_params.main.has_value()) {
                    auto &uid = scripting_params.main.value();
                    // run it during the first iteration of the update loop
                    run_on_game_thread([&uid]() { _run_init_script(uid); });
                }

                break;
            }
            case LifecycleStage::Deinit: {
                ScriptManager::instance().perform_deinit();
                break;
            }
            default:
                break;
        }
    }
}
