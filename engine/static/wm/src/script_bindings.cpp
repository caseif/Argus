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
    static void _bind_window_symbols(void) {
        bind_type<Window>("Window");

        static_assert(std::is_function_v<decltype(get_window)>);
        bind_member_static_function<Window>("get_window", get_window);

        //TODO: get_canvas
        bind_member_instance_function("get_id", &Window::get_id);
        bind_member_instance_function("is_created", &Window::is_created);
        bind_member_instance_function("is_ready", &Window::is_ready);
        bind_member_instance_function("create_child_window", &Window::create_child_window);
        bind_member_instance_function("remove_child", &Window::remove_child);
        bind_member_instance_function("set_title", &Window::set_title);
        bind_member_instance_function("is_fullscreen", &Window::is_fullscreen);
        bind_member_instance_function("set_fullscreen", &Window::set_fullscreen);
        bind_member_instance_function("get_windowed_resolution", &Window::get_windowed_resolution);
        bind_member_instance_function<void(Window::*)(unsigned int, unsigned int)>("set_windowed_resolution",
                &Window::set_windowed_resolution);
        bind_member_instance_function("set_vsync_enabled", &Window::set_vsync_enabled);
        bind_member_instance_function<void(Window::*)(int, int)>("set_windowed_position",
                &Window::set_windowed_position);
        //TODO: display functions
        bind_member_instance_function("is_mouse_captured", &Window::is_mouse_captured);
        bind_member_instance_function("set_mouse_captured", &Window::set_mouse_captured);
        bind_member_instance_function("is_mouse_raw_input", &Window::is_mouse_raw_input);
        bind_member_instance_function("set_mouse_raw_input", &Window::set_mouse_raw_input);
    }

    void register_wm_bindings(void) {
        _bind_window_symbols();
    }
}
