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
    constexpr const uint64_t k_ns_per_us = 1'000;
    constexpr const uint64_t k_ns_per_ms = 1'000'000;
    constexpr const uint64_t k_ns_per_s = 1'000'000'000;

    BindableTimeDelta::BindableTimeDelta(TimeDelta delta)
            : m_nanos(uint64_t(std::max<int64_t>(0, std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count()))) {
    }

    BindableTimeDelta::BindableTimeDelta(const BindableTimeDelta &rhs) = default;

    BindableTimeDelta::BindableTimeDelta(BindableTimeDelta &&rhs) = default;

    BindableTimeDelta &BindableTimeDelta::operator=(const BindableTimeDelta &rhs) {
        this->m_nanos = rhs.m_nanos;
        return *this;
    }

    BindableTimeDelta &BindableTimeDelta::operator=(BindableTimeDelta &&rhs) {
        this->m_nanos = rhs.m_nanos;
        return *this;
    }

    BindableTimeDelta::~BindableTimeDelta(void) = default;

    uint64_t BindableTimeDelta::nanos(void) const {
        return m_nanos;
    }

    uint64_t BindableTimeDelta::micros(void) const {
        return m_nanos / k_ns_per_us;
    }

    uint64_t BindableTimeDelta::millis(void) const {
        return m_nanos / k_ns_per_ms;
    }

    uint64_t BindableTimeDelta::seconds(void) const {
        return m_nanos / k_ns_per_s;
    }

    class BindableVector2d : Vector2d, ScriptBindable {
    };

    class BindableVector2f : Vector2f, ScriptBindable {
    };

    class BindableVector2i : Vector2i, ScriptBindable {
    };

    class BindableVector2u : Vector2u, ScriptBindable {
    };

    static void _bind_time_symbols(void) {
        auto delta_type = create_type_def<BindableTimeDelta>("TimeDelta");
        add_member_instance_function(delta_type, "nanos", &BindableTimeDelta::nanos);
        add_member_instance_function(delta_type, "micros", &BindableTimeDelta::micros);
        add_member_instance_function(delta_type, "millis", &BindableTimeDelta::millis);
        add_member_instance_function(delta_type, "seconds", &BindableTimeDelta::seconds);
        bind_type(delta_type);
    }

    static void _bind_math_symbols(void) {
        bind_type<BindableVector2d>("Vector2d");
        bind_type<BindableVector2f>("Vector2f");
        bind_type<BindableVector2i>("Vector2i");
        bind_type<BindableVector2u>("Vector2u");
    }

    void register_lowlevel_bindings(void) {
        _bind_time_symbols();
        _bind_math_symbols();
    }
}
