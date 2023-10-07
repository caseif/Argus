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

#include "argus/wm.hpp"
#include "internal/wm/script_bindings.hpp"

namespace argus {
    typedef std::function<void(const WindowEvent &)> WindowEventCallback;

    static void _bind_window_symbols(void) {
        bind_type<Window>("Window");

        static_assert(std::is_function_v<decltype(get_window)>);
        bind_member_static_function<Window>("get_window", get_window);

        // get_canvas needs to be bound in render since that's where it's defined
        bind_member_instance_function("get_id", &Window::get_id);
        bind_member_instance_function("is_created", &Window::is_created);
        bind_member_instance_function("is_ready", &Window::is_ready);
        bind_member_instance_function("create_child_window", &Window::create_child_window);
        bind_member_instance_function("remove_child", &Window::remove_child);
        bind_member_instance_function("set_title", &Window::set_title);
        bind_member_instance_function("is_fullscreen", &Window::is_fullscreen);
        bind_member_instance_function("set_fullscreen", &Window::set_fullscreen);
        //TODO: figure out a way to bind get_resolution
        bind_member_instance_function("peek_resolution", &Window::peek_resolution);
        bind_member_instance_function("get_windowed_resolution", &Window::get_windowed_resolution);
        bind_member_instance_function<void(Window::*)(unsigned int, unsigned int)>("set_windowed_resolution",
                &Window::set_windowed_resolution);
        //TODO: bind is_vsync_enabled
        bind_member_instance_function("set_vsync_enabled", &Window::set_vsync_enabled);
        bind_member_instance_function<void(Window::*)(int, int)>("set_windowed_position",
                &Window::set_windowed_position);
        bind_member_instance_function("get_display_affinity", &Window::get_display_affinity);
        bind_member_instance_function("set_display_affinity", &Window::set_display_affinity);
        bind_member_instance_function("get_display_mode", &Window::get_display_mode);
        bind_member_instance_function("set_display_mode", &Window::set_display_mode);
        bind_member_instance_function("is_mouse_captured", &Window::is_mouse_captured);
        bind_member_instance_function("set_mouse_captured", &Window::set_mouse_captured);
        bind_member_instance_function("is_mouse_visible", &Window::is_mouse_visible);
        bind_member_instance_function("set_mouse_visible", &Window::set_mouse_visible);
        bind_member_instance_function("is_mouse_raw_input", &Window::is_mouse_raw_input);
        bind_member_instance_function("set_mouse_raw_input", &Window::set_mouse_raw_input);
        bind_member_instance_function("get_content_scale", &Window::get_content_scale);
        bind_member_instance_function("commit", &Window::commit);
    }

    static void _bind_display_symbols(void) {
        bind_type<DisplayMode>("DisplayMode");
        bind_member_field("resolution", &DisplayMode::resolution);
        bind_member_field("refresh_rate", &DisplayMode::refresh_rate);
        bind_member_field("color_depth", &DisplayMode::color_depth);

        bind_type<Display>("Display");
        bind_member_instance_function("get_name", &Display::get_name);
        bind_member_instance_function("get_position", &Display::get_position);
    }

    static void _bind_window_event_symbols(void) {
        bind_enum<WindowEventType>("WindowEventType");
        bind_enum_value("Create", WindowEventType::Create);
        bind_enum_value("Update", WindowEventType::Update);
        bind_enum_value("RequestClose", WindowEventType::RequestClose);
        bind_enum_value("Minimize", WindowEventType::Minimize);
        bind_enum_value("Restore", WindowEventType::Restore);
        bind_enum_value("Focus", WindowEventType::Focus);
        bind_enum_value("Unfocus", WindowEventType::Unfocus);
        bind_enum_value("Resize", WindowEventType::Resize);
        bind_enum_value("Move", WindowEventType::Move);

        bind_type<WindowEvent>("WindowEvent");
        bind_member_field("type", &WindowEvent::subtype);
        bind_member_field("resolution", &WindowEvent::resolution);
        bind_member_field("position", &WindowEvent::position);
        bind_member_field("delta", &WindowEvent::delta);
        bind_extension_function<WindowEvent>("get_window",
                +[](const WindowEvent &event) -> Window & { return event.window; });

        bind_global_function<Index (*)(std::function<void(const WindowEvent &)>, TargetThread, Ordering)>(
                "register_window_event_handler", register_event_handler<WindowEvent>);
    }

    void register_wm_bindings(void) {
        _bind_window_symbols();
        _bind_display_symbols();
        _bind_window_event_symbols();
    }
}
