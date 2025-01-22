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

#include "argus/core/event.hpp"

#include "argus/scripting/bind.hpp"
#include "argus/scripting/manager.hpp"
#include "internal/scripting/core_bindings.hpp"

namespace argus {
    std::vector<ScriptDeltaCallback> g_update_callbacks;

    // value-typed param is necessary to be able to bind the function
    static void _script_register_update_callback(ScriptDeltaCallback callback) { // NOLINT(*-unnecessary-value-param)
        register_update_callback(callback);
    }

    static void _bind_engine_types(void) {
        auto &mgr = ScriptManager::instance();

        auto tt_enum_def = create_enum_def<TargetThread>("TargetThread").expect();
        add_enum_value(tt_enum_def, "Update", TargetThread::Update).expect();
        add_enum_value(tt_enum_def, "Render", TargetThread::Render).expect();
        mgr.bind_enum(tt_enum_def).expect();

        auto ordering_enum_def = create_enum_def<Ordering>("Ordering").expect();
        add_enum_value(ordering_enum_def, "First", Ordering::First).expect();
        add_enum_value(ordering_enum_def, "Early", Ordering::Early).expect();
        add_enum_value(ordering_enum_def, "Standard", Ordering::Standard).expect();
        add_enum_value(ordering_enum_def, "Late", Ordering::Late).expect();
        add_enum_value(ordering_enum_def, "Last", Ordering::Last).expect();
        mgr.bind_enum(ordering_enum_def).expect();
    }

    static void _bind_engine_functions(void) {
        ScriptManager::instance().bind_global_function(create_global_function_def(
                "register_update_callback",
                _script_register_update_callback
        ).expect()).expect();
    }

    void register_core_bindings(void) {
        _bind_engine_types();
        _bind_engine_functions();
    }
}
