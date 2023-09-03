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

#include "argus/core/event.hpp"

#include "argus/scripting/bind.hpp"
#include "internal/scripting/core_bindings.hpp"

#include <algorithm>
#include <chrono>

#include <cstdint>

namespace argus {
    std::vector<ScriptDeltaCallback> g_update_callbacks;

    static void _script_register_update_callback(ScriptDeltaCallback callback) {
        g_update_callbacks.push_back(callback);
    }

    static void _bind_engine_symbols(void) {
        bind_global_function("register_update_callback", _script_register_update_callback);
    }

    struct Foo : ScriptBindable {
        std::string bar;
        Foo(std::string str) : bar(str) {
        }
    };

    static Foo foo("Hello World!");

    static Foo &get_foo(void) {
        return foo;
    }

    void register_core_bindings(void) {
        _bind_engine_symbols();

        bind_type<Foo>("Foo");
        bind_global_function("get_foo", get_foo);
        bind_member_field("bar", &Foo::bar);
    }

    void invoke_update_callbacks(TimeDelta delta) {
        printf("C++: %s\n", foo.bar.c_str());
        for (const auto &callback : g_update_callbacks) {
            callback(BindableTimeDelta(delta));
        }
    }
}
