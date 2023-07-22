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
#include "internal/wm/script_bindings.hpp"

namespace argus {
    static void _bind_window_type(void) {
        auto def = create_type_def<Window>("Window");

        add_member_static_function(def, "get_window", get_window);

        //TODO: get_canvas
        add_member_instance_function(def, "get_id", &Window::get_id);
        add_member_instance_function(def, "is_created", &Window::is_created);
        add_member_instance_function(def, "is_ready", &Window::is_ready);
        add_member_instance_function(def, "create_child_window", &Window::create_child_window);
        add_member_instance_function(def, "remove_child", &Window::remove_child);
        add_member_instance_function(def, "set_title", &Window::set_title);
        add_member_instance_function(def, "is_fullscreen", &Window::is_fullscreen);
        add_member_instance_function(def, "set_fullscreen", &Window::set_fullscreen);
        //TODO: resolution functions
        add_member_instance_function<void(Window::*)(unsigned int, unsigned int)>(def, "set_windowed_resolution",
                &Window::set_windowed_resolution);
        add_member_instance_function(def, "set_vsync_enabled", &Window::set_vsync_enabled);
        add_member_instance_function<void(Window::*)(int, int)>(def, "set_windowed_position",
                &Window::set_windowed_position);
        //TODO: display functions
        add_member_instance_function(def, "is_mouse_captured", &Window::is_mouse_captured);
        add_member_instance_function(def, "set_mouse_captured", &Window::set_mouse_captured);
        add_member_instance_function(def, "is_mouse_raw_input", &Window::is_mouse_raw_input);
        add_member_instance_function(def, "set_mouse_raw_input", &Window::set_mouse_raw_input);

        bind_type(def);
    }

    void register_wm_bindings(void) {
        _bind_window_type();
    }
}
