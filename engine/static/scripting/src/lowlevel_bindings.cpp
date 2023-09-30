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
    static int64_t _nanos_to_micros(const std::chrono::nanoseconds &ns) {
        return std::chrono::duration_cast<std::chrono::microseconds>(ns).count();
    }

    static int64_t _nanos_to_millis(const std::chrono::nanoseconds &ns) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(ns).count();
    }

    static int64_t _nanos_to_seconds(const std::chrono::nanoseconds &ns) {
        return std::chrono::duration_cast<std::chrono::seconds>(ns).count();
    }

    static void _bind_time_symbols(void) {
        bind_type<TimeDelta>("TimeDelta");
        bind_member_instance_function("nanos", &TimeDelta::count);
        bind_extension_function<TimeDelta>("micros", &_nanos_to_micros);
        bind_extension_function<TimeDelta>("millis", &_nanos_to_millis);
        bind_extension_function<TimeDelta>("seconds", &_nanos_to_seconds);
    }

    static void _bind_math_symbols(void) {
        bind_type<Vector2d>("Vector2d");
        bind_type<Vector2f>("Vector2f");
        bind_type<Vector2i>("Vector2i");
        bind_type<Vector2u>("Vector2u");
        bind_type<Vector3d>("Vector3d");
        bind_type<Vector3f>("Vector3f");
        bind_type<Vector3i>("Vector3i");
        bind_type<Vector3u>("Vector3u");
        bind_type<Vector4d>("Vector4d");
        bind_type<Vector4f>("Vector4f");
        bind_type<Vector4i>("Vector4i");
        bind_type<Vector4u>("Vector4u");
    }

    void register_lowlevel_bindings(void) {
        _bind_time_symbols();
        _bind_math_symbols();
    }
}
