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

#include "argus/scripting.hpp"

#include "argus/wm.hpp"
#include "internal/wm/script_bindings.hpp"

namespace argus {
    typedef std::function<void(const WindowEvent &)> WindowEventCallback;

    static void _bind_window_symbols(void) {
        bind_type<Window>("Window").expect();

        static_assert(std::is_function_v<decltype(get_window)>);
        bind_member_static_function<Window>("get_window", get_window).expect();

        // get_canvas needs to be bound in render since that's where it's defined
        bind_member_instance_function("get_id", &Window::get_id).expect();
        bind_member_instance_function("is_created", &Window::is_created).expect();
        bind_member_instance_function("is_ready", &Window::is_ready).expect();
        bind_member_instance_function("create_child_window", &Window::create_child_window).expect();
        bind_member_instance_function("remove_child", &Window::remove_child).expect();
        bind_member_instance_function("set_title", &Window::set_title).expect();
        bind_member_instance_function("is_fullscreen", &Window::is_fullscreen).expect();
        bind_member_instance_function("set_fullscreen", &Window::set_fullscreen).expect();
        //TODO: figure out a way to bind get_resolution
        bind_member_instance_function("peek_resolution", &Window::peek_resolution).expect();
        bind_member_instance_function("get_windowed_resolution", &Window::get_windowed_resolution).expect();
        bind_member_instance_function<void (Window::*)(unsigned int, unsigned int)>("set_windowed_resolution",
                &Window::set_windowed_resolution).expect();
        //TODO: bind is_vsync_enabled
        bind_member_instance_function("set_vsync_enabled", &Window::set_vsync_enabled).expect();
        bind_member_instance_function<void (Window::*)(int, int)>("set_windowed_position",
                &Window::set_windowed_position).expect();
        bind_member_instance_function("get_display_affinity", &Window::get_display_affinity).expect();
        bind_member_instance_function("set_display_affinity", &Window::set_display_affinity).expect();
        bind_member_instance_function("get_display_mode", &Window::get_display_mode).expect();
        bind_member_instance_function("set_display_mode", &Window::set_display_mode).expect();
        bind_member_instance_function("is_mouse_captured", &Window::is_mouse_captured).expect();
        bind_member_instance_function("set_mouse_captured", &Window::set_mouse_captured).expect();
        bind_member_instance_function("is_mouse_visible", &Window::is_mouse_visible).expect();
        bind_member_instance_function("set_mouse_visible", &Window::set_mouse_visible).expect();
        bind_member_instance_function("is_mouse_raw_input", &Window::is_mouse_raw_input).expect();
        bind_member_instance_function("set_mouse_raw_input", &Window::set_mouse_raw_input).expect();
        bind_member_instance_function("get_content_scale", &Window::get_content_scale).expect();
        bind_member_instance_function("commit", &Window::commit).expect();
    }

    static void _bind_display_symbols(void) {
        bind_type<DisplayMode>("DisplayMode").expect();
        bind_member_field("resolution", &DisplayMode::resolution).expect();
        bind_member_field("refresh_rate", &DisplayMode::refresh_rate).expect();
        bind_member_field("color_depth", &DisplayMode::color_depth).expect();

        bind_type<Display>("Display").expect();
        bind_member_instance_function("get_name", &Display::get_name).expect();
        bind_member_instance_function("get_position", &Display::get_position).expect();
    }

    static void _bind_window_event_symbols(void) {
        bind_enum<WindowEventType>("WindowEventType").expect();
        bind_enum_value("Create", WindowEventType::Create).expect();
        bind_enum_value("Update", WindowEventType::Update).expect();
        bind_enum_value("RequestClose", WindowEventType::RequestClose).expect();
        bind_enum_value("Minimize", WindowEventType::Minimize).expect();
        bind_enum_value("Restore", WindowEventType::Restore).expect();
        bind_enum_value("Focus", WindowEventType::Focus).expect();
        bind_enum_value("Unfocus", WindowEventType::Unfocus).expect();
        bind_enum_value("Resize", WindowEventType::Resize).expect();
        bind_enum_value("Move", WindowEventType::Move).expect();

        bind_type<WindowEvent>("WindowEvent").expect();
        bind_member_field("type", &WindowEvent::subtype).expect();
        bind_member_field("resolution", &WindowEvent::resolution).expect();
        bind_member_field("position", &WindowEvent::position).expect();
        bind_member_field("delta", &WindowEvent::delta).expect();
        bind_extension_function<WindowEvent>("get_window",
                +[](const WindowEvent &event) -> Window & { return event.window; }).expect();

        bind_global_function<Index (*)(std::function<void(const WindowEvent &)>, TargetThread, Ordering)>(
                "register_window_event_handler", register_event_handler<WindowEvent>).expect();
    }

    void register_wm_bindings(void) {
        _bind_window_symbols();
        _bind_display_symbols();
        _bind_window_event_symbols();
    }
}
