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

#include "argus/scripting/bind.hpp"
#include "internal/scripting/callback_bindings.hpp"
#include "argus/core/event.hpp"

#include <algorithm>
#include <chrono>

#include <cstdint>

namespace argus {
    constexpr const uint64_t k_ns_per_us = 1'000;
    constexpr const uint64_t k_ns_per_ms = 1'000'000;
    constexpr const uint64_t k_ns_per_s = 1'000'000'000;

    std::vector<ScriptDeltaCallback> g_update_callbacks;

    BindableTimeDelta::BindableTimeDelta(TimeDelta delta)
        : m_nanos(uint64_t(std::max<long>(0L, std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count()))) {
    }

    BindableTimeDelta::BindableTimeDelta(const BindableTimeDelta &rhs) = default;

    BindableTimeDelta::~BindableTimeDelta(void) = default;

    uint64_t BindableTimeDelta::nanos(void) {
        return m_nanos;
    }

    uint64_t BindableTimeDelta::micros(void) {
        return m_nanos / k_ns_per_us;
    }

    uint64_t BindableTimeDelta::millis(void) {
        return m_nanos / k_ns_per_ms;
    }

    uint64_t BindableTimeDelta::seconds(void) {
        return m_nanos / k_ns_per_s;
    }

    static void _script_register_update_callback(ScriptDeltaCallback callback) {
        g_update_callbacks.push_back(callback);
    }

    void register_default_bindings(void) {
        auto delta_type = create_type_def<BindableTimeDelta>("TimeDelta");
        add_member_instance_function(delta_type, "nanos", &BindableTimeDelta::nanos);
        add_member_instance_function(delta_type, "micros", &BindableTimeDelta::micros);
        add_member_instance_function(delta_type, "millis", &BindableTimeDelta::millis);
        add_member_instance_function(delta_type, "seconds", &BindableTimeDelta::seconds);
        bind_type(delta_type);

        bind_global_function("register_update_callback", _script_register_update_callback);
    }

    void invoke_update_callbacks(TimeDelta delta) {
        for (const auto &callback : g_update_callbacks) {
            callback(BindableTimeDelta(delta));
        }
    }
}
