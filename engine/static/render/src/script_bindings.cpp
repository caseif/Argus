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

#include "argus/scripting.hpp"

#include "argus/wm/window.hpp"

#include "argus/render/common/canvas.hpp"
#include "argus/render/common/transform.hpp"
#include "internal/render/script_bindings.hpp"

namespace argus {
    static void _register_transform_symbols(void) {
        bind_type<Transform2D>("Transform2D");
        bind_member_static_function<Transform2D>("new", +[](void) -> Transform2D { return Transform2D(); });
        bind_member_static_function<Transform2D>("of",
                +[](const Vector2f translation, float rotation_rads, const Vector2f &scale) -> Transform2D {
                    return Transform2D(translation, rotation_rads, scale);
                });
        bind_member_instance_function("get_translation", &Transform2D::get_translation);
        bind_member_instance_function("get_rotation", &Transform2D::get_rotation);
        bind_member_instance_function("get_scale", &Transform2D::get_scale);
        bind_member_instance_function<void (Transform2D::*)(float x, float y)>("set_translation",
                &Transform2D::set_translation);
        bind_member_instance_function("set_rotation", &Transform2D::set_rotation);
        bind_member_instance_function<void (Transform2D::*)(float x, float y)>("set_scale", &Transform2D::set_scale);
        bind_member_instance_function<void (Transform2D::*)(float dx, float dy)>("add_translation",
                &Transform2D::add_translation);
        bind_member_instance_function("add_rotation",
                &Transform2D::add_rotation);
        bind_extension_function<Transform2D>("x",
                +[](const Transform2D &transform) { return transform.get_translation().x; });
        bind_extension_function<Transform2D>("y",
                +[](const Transform2D &transform) { return transform.get_translation().y; });
        bind_extension_function<Transform2D>("sx",
                +[](const Transform2D &transform) { return transform.get_scale().x; });
        bind_extension_function<Transform2D>("sy",
                +[](const Transform2D &transform) { return transform.get_scale().y; });
    }

    static void _register_canvas_symbols(void) {
        bind_type<Canvas>("Canvas");
        bind_member_instance_function("get_window", &Canvas::get_window);
        // other Canvas functions are intended for use by downstream modules and
        // so are not bound

        bind_member_instance_function("get_canvas", &Window::get_canvas);
    }

    void register_render_script_bindings(void) {
        _register_transform_symbols();
        _register_canvas_symbols();
    }
}
