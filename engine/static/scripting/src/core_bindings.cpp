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
#include "internal/scripting/core_bindings.hpp"

#include <chrono>

namespace argus {
    std::vector<ScriptDeltaCallback> g_update_callbacks;

    static void _script_register_update_callback(ScriptDeltaCallback callback) {
        register_update_callback(callback);
    }

    static void _bind_engine_types(void) {
        bind_enum<TargetThread>("TargetThread");
        bind_enum_value("Update", TargetThread::Update);
        bind_enum_value("Render", TargetThread::Render);

        bind_enum<Ordering>("Ordering");
        bind_enum_value("First", Ordering::First);
        bind_enum_value("Early", Ordering::Early);
        bind_enum_value("Standard", Ordering::Standard);
        bind_enum_value("Late", Ordering::Late);
        bind_enum_value("Last", Ordering::Last);
    }

    static void _bind_engine_functions(void) {
        bind_global_function("register_update_callback", _script_register_update_callback);
    }

    void register_core_bindings(void) {
        _bind_engine_types();
        _bind_engine_functions();
    }
}
