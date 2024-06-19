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

#include "argus/lowlevel/handle.hpp"
#include "argus/lowlevel/math.hpp"

#include "argus/scripting/bind.hpp"
#include "internal/scripting/core_bindings.hpp"

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
        bind_type<TimeDelta>("TimeDelta").expect();
        bind_member_instance_function("nanos", &TimeDelta::count).expect();
        bind_extension_function<TimeDelta>("micros", &_nanos_to_micros).expect();
        bind_extension_function<TimeDelta>("millis", &_nanos_to_millis).expect();
        bind_extension_function<TimeDelta>("seconds", &_nanos_to_seconds).expect();
    }

    template<typename V>
    static std::enable_if_t<std::is_arithmetic_v<typename V::element_type>, void>
    _bind_vector2(const std::string &name) {
        using E = typename V::element_type;
        bind_type<V>(name).expect();
        bind_member_field("x", &V::x).expect();
        bind_member_field("y", &V::y).expect();
        bind_member_static_function<V>("new", +[](void) -> V { return V(); }).expect();
        bind_member_static_function<V>("of", +[](E x, E y) -> V { return V(x, y); }).expect();
    }

    template<typename V>
    static std::enable_if_t<std::is_arithmetic_v<typename V::element_type>, void>
    _bind_vector3(const std::string &name) {
        using E = typename V::element_type;
        bind_type<V>(name).expect();
        bind_member_field("x", &V::x).expect();
        bind_member_field("y", &V::y).expect();
        bind_member_field("z", &V::z).expect();
        bind_member_static_function<V>("new", +[](void) -> V { return V(); }).expect();
        bind_member_static_function<V>("of", +[](E x, E y, E z) -> V { return V(x, y, z); }).expect();
    }

    template<typename V>
    static std::enable_if_t<std::is_arithmetic_v<typename V::element_type>, void>
    _bind_vector4(const std::string &name) {
        using E = typename V::element_type;
        bind_type<V>(name).expect();
        bind_member_field("x", &V::x).expect();
        bind_member_field("y", &V::y).expect();
        bind_member_field("z", &V::z).expect();
        bind_member_field("w", &V::w).expect();
        bind_member_static_function<V>("new", +[](void) -> V { return V(); }).expect();
        bind_member_static_function<V>("of", +[](E x, E y, E z, E w) -> V { return V(x, y, z, w); }).expect();
    }

    static void _bind_math_symbols(void) {
        _bind_vector2<Vector2d>("Vector2d");
        _bind_vector2<Vector2f>("Vector2f");
        _bind_vector2<Vector2i>("Vector2i");
        _bind_vector2<Vector2u>("Vector2u");
        _bind_vector3<Vector3d>("Vector3d");
        _bind_vector3<Vector3f>("Vector3f");
        _bind_vector3<Vector3i>("Vector3i");
        _bind_vector3<Vector3u>("Vector3u");
        _bind_vector4<Vector4d>("Vector4d");
        _bind_vector4<Vector4f>("Vector4f");
        _bind_vector4<Vector4i>("Vector4i");
        _bind_vector4<Vector4u>("Vector4u");

        bind_type<Padding>("Padding").expect();
        bind_member_field("top", &Padding::top).expect();
        bind_member_field("bottom", &Padding::bottom).expect();
        bind_member_field("left", &Padding::left).expect();
        bind_member_field("right", &Padding::right).expect();
    }

    static void _bind_handle_symbols(void) {
        bind_type<Handle>("Handle").expect();
    }

    void register_lowlevel_bindings(void) {
        _bind_time_symbols();
        _bind_math_symbols();
        _bind_handle_symbols();
    }
}
