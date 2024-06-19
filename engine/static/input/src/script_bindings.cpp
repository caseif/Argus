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

#include "argus/input.hpp"
#include "internal/input/script_bindings.hpp"

namespace argus {
    static void _bind_input_manager_symbols(void) {
        bind_type<input::InputManager>("InputManager").expect();
        bind_member_instance_function("get_controller", &input::InputManager::get_controller).expect();
        bind_member_instance_function("add_controller", &input::InputManager::add_controller).expect();
        bind_member_instance_function<void (input::InputManager::*)(const std::string &)>("remove_controller",
                &input::InputManager::remove_controller).expect();

        bind_member_instance_function("get_global_deadzone_radius",
                &input::InputManager::get_global_deadzone_radius).expect();
        bind_member_instance_function("set_global_deadzone_radius",
                &input::InputManager::set_global_deadzone_radius).expect();
        bind_member_instance_function("get_global_deadzone_shape",
                &input::InputManager::get_global_deadzone_shape).expect();
        bind_member_instance_function("set_global_deadzone_shape",
                &input::InputManager::set_global_deadzone_shape).expect();
        bind_member_instance_function("get_global_axis_deadzone_radius",
                &input::InputManager::get_global_axis_deadzone_radius).expect();
        bind_member_instance_function("set_global_axis_deadzone_radius",
                &input::InputManager::set_global_axis_deadzone_radius).expect();
        bind_member_instance_function("clear_global_axis_deadzone_radius",
                &input::InputManager::clear_global_axis_deadzone_radius).expect();
        bind_member_instance_function("get_global_axis_deadzone_shape",
                &input::InputManager::get_global_axis_deadzone_shape).expect();
        bind_member_instance_function("set_global_axis_deadzone_shape",
                &input::InputManager::set_global_axis_deadzone_shape).expect();
        bind_member_instance_function("clear_global_axis_deadzone_shape",
                &input::InputManager::clear_global_axis_deadzone_shape).expect();

        bind_global_function("get_input_manager", &input::InputManager::instance).expect();
    }

    static void _bind_keyboard_symbols(void) {
        bind_enum<input::KeyboardScancode>("KeyboardScancode").expect();
        bind_enum_value("Unknown", input::KeyboardScancode::Unknown).expect();
        bind_enum_value("A", input::KeyboardScancode::A).expect();
        bind_enum_value("B", input::KeyboardScancode::B).expect();
        bind_enum_value("C", input::KeyboardScancode::C).expect();
        bind_enum_value("D", input::KeyboardScancode::D).expect();
        bind_enum_value("E", input::KeyboardScancode::E).expect();
        bind_enum_value("F", input::KeyboardScancode::F).expect();
        bind_enum_value("G", input::KeyboardScancode::G).expect();
        bind_enum_value("H", input::KeyboardScancode::H).expect();
        bind_enum_value("I", input::KeyboardScancode::I).expect();
        bind_enum_value("J", input::KeyboardScancode::J).expect();
        bind_enum_value("K", input::KeyboardScancode::K).expect();
        bind_enum_value("L", input::KeyboardScancode::L).expect();
        bind_enum_value("M", input::KeyboardScancode::M).expect();
        bind_enum_value("N", input::KeyboardScancode::N).expect();
        bind_enum_value("O", input::KeyboardScancode::O).expect();
        bind_enum_value("P", input::KeyboardScancode::P).expect();
        bind_enum_value("Q", input::KeyboardScancode::Q).expect();
        bind_enum_value("R", input::KeyboardScancode::R).expect();
        bind_enum_value("S", input::KeyboardScancode::S).expect();
        bind_enum_value("T", input::KeyboardScancode::T).expect();
        bind_enum_value("U", input::KeyboardScancode::U).expect();
        bind_enum_value("V", input::KeyboardScancode::V).expect();
        bind_enum_value("W", input::KeyboardScancode::W).expect();
        bind_enum_value("X", input::KeyboardScancode::X).expect();
        bind_enum_value("Y", input::KeyboardScancode::Y).expect();
        bind_enum_value("Z", input::KeyboardScancode::Z).expect();
        bind_enum_value("Number1", input::KeyboardScancode::Number1).expect();
        bind_enum_value("Number2", input::KeyboardScancode::Number2).expect();
        bind_enum_value("Number3", input::KeyboardScancode::Number3).expect();
        bind_enum_value("Number4", input::KeyboardScancode::Number4).expect();
        bind_enum_value("Number5", input::KeyboardScancode::Number5).expect();
        bind_enum_value("Number6", input::KeyboardScancode::Number6).expect();
        bind_enum_value("Number7", input::KeyboardScancode::Number7).expect();
        bind_enum_value("Number8", input::KeyboardScancode::Number8).expect();
        bind_enum_value("Number9", input::KeyboardScancode::Number9).expect();
        bind_enum_value("Number0", input::KeyboardScancode::Number0).expect();
        bind_enum_value("Enter", input::KeyboardScancode::Enter).expect();
        bind_enum_value("Escape", input::KeyboardScancode::Escape).expect();
        bind_enum_value("Backspace", input::KeyboardScancode::Backspace).expect();
        bind_enum_value("Tab", input::KeyboardScancode::Tab).expect();
        bind_enum_value("Space", input::KeyboardScancode::Space).expect();
        bind_enum_value("Minus", input::KeyboardScancode::Minus).expect();
        bind_enum_value("Equals", input::KeyboardScancode::Equals).expect();
        bind_enum_value("LeftBracket", input::KeyboardScancode::LeftBracket).expect();
        bind_enum_value("RightBracket", input::KeyboardScancode::RightBracket).expect();
        bind_enum_value("BackSlash", input::KeyboardScancode::BackSlash).expect();
        bind_enum_value("Semicolon", input::KeyboardScancode::Semicolon).expect();
        bind_enum_value("Apostrophe", input::KeyboardScancode::Apostrophe).expect();
        bind_enum_value("Grave", input::KeyboardScancode::Grave).expect();
        bind_enum_value("Comma", input::KeyboardScancode::Comma).expect();
        bind_enum_value("Period", input::KeyboardScancode::Period).expect();
        bind_enum_value("ForwardSlash", input::KeyboardScancode::ForwardSlash).expect();
        bind_enum_value("CapsLock", input::KeyboardScancode::CapsLock).expect();
        bind_enum_value("F1", input::KeyboardScancode::F1).expect();
        bind_enum_value("F2", input::KeyboardScancode::F2).expect();
        bind_enum_value("F3", input::KeyboardScancode::F3).expect();
        bind_enum_value("F4", input::KeyboardScancode::F4).expect();
        bind_enum_value("F6", input::KeyboardScancode::F6).expect();
        bind_enum_value("F7", input::KeyboardScancode::F7).expect();
        bind_enum_value("F8", input::KeyboardScancode::F8).expect();
        bind_enum_value("F5", input::KeyboardScancode::F5).expect();
        bind_enum_value("F9", input::KeyboardScancode::F9).expect();
        bind_enum_value("F10", input::KeyboardScancode::F10).expect();
        bind_enum_value("F11", input::KeyboardScancode::F11).expect();
        bind_enum_value("F12", input::KeyboardScancode::F12).expect();
        bind_enum_value("PrintScreen", input::KeyboardScancode::PrintScreen).expect();
        bind_enum_value("ScrollLock", input::KeyboardScancode::ScrollLock).expect();
        bind_enum_value("Pause", input::KeyboardScancode::Pause).expect();
        bind_enum_value("Insert", input::KeyboardScancode::Insert).expect();
        bind_enum_value("Home", input::KeyboardScancode::Home).expect();
        bind_enum_value("PageUp", input::KeyboardScancode::PageUp).expect();
        bind_enum_value("Delete", input::KeyboardScancode::Delete).expect();
        bind_enum_value("End", input::KeyboardScancode::End).expect();
        bind_enum_value("PageDown", input::KeyboardScancode::PageDown).expect();
        bind_enum_value("ArrowRight", input::KeyboardScancode::ArrowRight).expect();
        bind_enum_value("ArrowLeft", input::KeyboardScancode::ArrowLeft).expect();
        bind_enum_value("ArrowDown", input::KeyboardScancode::ArrowDown).expect();
        bind_enum_value("ArrowUp", input::KeyboardScancode::ArrowUp).expect();
        bind_enum_value("NumpadNumLock", input::KeyboardScancode::NumpadNumLock).expect();
        bind_enum_value("NumpadDivide", input::KeyboardScancode::NumpadDivide).expect();
        bind_enum_value("NumpadTimes", input::KeyboardScancode::NumpadTimes).expect();
        bind_enum_value("NumpadMinus", input::KeyboardScancode::NumpadMinus).expect();
        bind_enum_value("NumpadPlus", input::KeyboardScancode::NumpadPlus).expect();
        bind_enum_value("NumpadEnter", input::KeyboardScancode::NumpadEnter).expect();
        bind_enum_value("Numpad1", input::KeyboardScancode::Numpad1).expect();
        bind_enum_value("Numpad2", input::KeyboardScancode::Numpad2).expect();
        bind_enum_value("Numpad3", input::KeyboardScancode::Numpad3).expect();
        bind_enum_value("Numpad4", input::KeyboardScancode::Numpad4).expect();
        bind_enum_value("Numpad5", input::KeyboardScancode::Numpad5).expect();
        bind_enum_value("Numpad6", input::KeyboardScancode::Numpad6).expect();
        bind_enum_value("Numpad7", input::KeyboardScancode::Numpad7).expect();
        bind_enum_value("Numpad8", input::KeyboardScancode::Numpad8).expect();
        bind_enum_value("Numpad9", input::KeyboardScancode::Numpad9).expect();
        bind_enum_value("Numpad0", input::KeyboardScancode::Numpad0).expect();
        bind_enum_value("NumpadDot", input::KeyboardScancode::NumpadDot).expect();
        bind_enum_value("NumpadEquals", input::KeyboardScancode::NumpadEquals).expect();
        bind_enum_value("Menu", input::KeyboardScancode::Menu).expect();
        bind_enum_value("LeftControl", input::KeyboardScancode::LeftControl).expect();
        bind_enum_value("LeftShift", input::KeyboardScancode::LeftShift).expect();
        bind_enum_value("LeftAlt", input::KeyboardScancode::LeftAlt).expect();
        bind_enum_value("Super", input::KeyboardScancode::Super).expect();
        bind_enum_value("RightControl", input::KeyboardScancode::RightControl).expect();
        bind_enum_value("RightShift", input::KeyboardScancode::RightShift).expect();
        bind_enum_value("RightAlt", input::KeyboardScancode::RightAlt).expect();

        bind_enum<input::KeyboardCommand>("KeyboardCommand").expect();
        bind_enum_value("Escape", input::KeyboardCommand::Escape).expect();
        bind_enum_value("F1", input::KeyboardCommand::F1).expect();
        bind_enum_value("F2", input::KeyboardCommand::F2).expect();
        bind_enum_value("F3", input::KeyboardCommand::F3).expect();
        bind_enum_value("F4", input::KeyboardCommand::F4).expect();
        bind_enum_value("F5", input::KeyboardCommand::F5).expect();
        bind_enum_value("F6", input::KeyboardCommand::F6).expect();
        bind_enum_value("F7", input::KeyboardCommand::F7).expect();
        bind_enum_value("F8", input::KeyboardCommand::F8).expect();
        bind_enum_value("F9", input::KeyboardCommand::F9).expect();
        bind_enum_value("F10", input::KeyboardCommand::F10).expect();
        bind_enum_value("F11", input::KeyboardCommand::F11).expect();
        bind_enum_value("F12", input::KeyboardCommand::F12).expect();
        bind_enum_value("Backspace", input::KeyboardCommand::Backspace).expect();
        bind_enum_value("Tab", input::KeyboardCommand::Tab).expect();
        bind_enum_value("CapsLock", input::KeyboardCommand::CapsLock).expect();
        bind_enum_value("Enter", input::KeyboardCommand::Enter).expect();
        bind_enum_value("Menu", input::KeyboardCommand::Menu).expect();
        bind_enum_value("PrintScreen", input::KeyboardCommand::PrintScreen).expect();
        bind_enum_value("ScrollLock", input::KeyboardCommand::ScrollLock).expect();
        bind_enum_value("Break", input::KeyboardCommand::Break).expect();
        bind_enum_value("Insert", input::KeyboardCommand::Insert).expect();
        bind_enum_value("Home", input::KeyboardCommand::Home).expect();
        bind_enum_value("PageUp", input::KeyboardCommand::PageUp).expect();
        bind_enum_value("Delete", input::KeyboardCommand::Delete).expect();
        bind_enum_value("End", input::KeyboardCommand::End).expect();
        bind_enum_value("PageDown", input::KeyboardCommand::PageDown).expect();
        bind_enum_value("ArrowUp", input::KeyboardCommand::ArrowUp).expect();
        bind_enum_value("ArrowLeft", input::KeyboardCommand::ArrowLeft).expect();
        bind_enum_value("ArrowDown", input::KeyboardCommand::ArrowDown).expect();
        bind_enum_value("ArrowRight", input::KeyboardCommand::ArrowRight).expect();
        bind_enum_value("NumpadNumLock", input::KeyboardCommand::NumpadNumLock).expect();
        bind_enum_value("NumpadEnter", input::KeyboardCommand::NumpadEnter).expect();
        bind_enum_value("NumpadDot", input::KeyboardCommand::NumpadDot).expect();
        bind_enum_value("Super", input::KeyboardCommand::Super).expect();

        bind_enum<input::KeyboardModifiers>("KeyboardModifiers").expect();
        bind_enum_value("None", input::KeyboardModifiers::None).expect();
        bind_enum_value("Shift", input::KeyboardModifiers::Shift).expect();
        bind_enum_value("Control", input::KeyboardModifiers::Control).expect();
        bind_enum_value("Super", input::KeyboardModifiers::Super).expect();
        bind_enum_value("Alt", input::KeyboardModifiers::Alt).expect();

        bind_global_function("get_key_name", input::get_key_name).expect();
        bind_global_function("is_key_pressed", input::is_key_pressed).expect();
    }

    static void _bind_mouse_symbols(void) {
        bind_enum<input::MouseButton>("MouseButton").expect();
        bind_enum_value("Primary", input::MouseButton::Primary).expect();
        bind_enum_value("Secondary", input::MouseButton::Secondary).expect();
        bind_enum_value("Middle", input::MouseButton::Middle).expect();
        bind_enum_value("Back", input::MouseButton::Back).expect();
        bind_enum_value("Forward", input::MouseButton::Forward).expect();

        bind_enum<input::MouseAxis>("MouseAxis").expect();
        bind_enum_value("Horizontal", input::MouseAxis::Horizontal).expect();
        bind_enum_value("Vertical", input::MouseAxis::Vertical).expect();

        bind_global_function("mouse_delta", input::mouse_delta).expect();
        bind_global_function("mouse_pos", input::mouse_pos).expect();
    }

    static void _bind_gamepad_symbols(void) {
        bind_enum<input::GamepadButton>("GamepadButton").expect();
        bind_enum_value("Unknown", input::GamepadButton::Unknown).expect();
        bind_enum_value("A", input::GamepadButton::A).expect();
        bind_enum_value("B", input::GamepadButton::B).expect();
        bind_enum_value("X", input::GamepadButton::X).expect();
        bind_enum_value("Y", input::GamepadButton::Y).expect();
        bind_enum_value("DpadUp", input::GamepadButton::DpadUp).expect();
        bind_enum_value("DpadDown", input::GamepadButton::DpadDown).expect();
        bind_enum_value("DpadLeft", input::GamepadButton::DpadLeft).expect();
        bind_enum_value("DpadRight", input::GamepadButton::DpadRight).expect();
        bind_enum_value("LBumper", input::GamepadButton::LBumper).expect();
        bind_enum_value("RBumper", input::GamepadButton::RBumper).expect();
        bind_enum_value("LTrigger", input::GamepadButton::LTrigger).expect();
        bind_enum_value("RTrigger", input::GamepadButton::RTrigger).expect();
        bind_enum_value("LStick", input::GamepadButton::LStick).expect();
        bind_enum_value("RStick", input::GamepadButton::RStick).expect();
        bind_enum_value("L4", input::GamepadButton::L4).expect();
        bind_enum_value("R4", input::GamepadButton::R4).expect();
        bind_enum_value("L5", input::GamepadButton::L5).expect();
        bind_enum_value("R5", input::GamepadButton::R5).expect();
        bind_enum_value("Start", input::GamepadButton::Start).expect();
        bind_enum_value("Back", input::GamepadButton::Back).expect();
        bind_enum_value("Guide", input::GamepadButton::Guide).expect();
        bind_enum_value("Misc1", input::GamepadButton::Misc1).expect();
        bind_enum_value("MaxValue", input::GamepadButton::MaxValue).expect();

        bind_enum<input::GamepadAxis>("GamepadAxis").expect();
        bind_enum_value("Unknown", input::GamepadAxis::Unknown).expect();
        bind_enum_value("LeftX", input::GamepadAxis::LeftX).expect();
        bind_enum_value("LeftY", input::GamepadAxis::LeftY).expect();
        bind_enum_value("RightX", input::GamepadAxis::RightX).expect();
        bind_enum_value("RightY", input::GamepadAxis::RightY).expect();
        bind_enum_value("LTrigger", input::GamepadAxis::LTrigger).expect();
        bind_enum_value("RTrigger", input::GamepadAxis::RTrigger).expect();
        bind_enum_value("MaxValue", input::GamepadAxis::MaxValue).expect();

        bind_global_function("get_gamepad_name", input::get_gamepad_name).expect();
        bind_global_function("is_gamepad_button_pressed", input::is_gamepad_button_pressed).expect();
        bind_global_function("get_gamepad_axis", input::get_gamepad_axis).expect();

        bind_global_function("get_connected_gamepad_count", &input::get_connected_gamepad_count).expect();
        bind_global_function("get_unattached_gamepad_count", &input::get_unattached_gamepad_count).expect();
    }

    static void _bind_controller_symbols(void) {
        bind_enum<input::DeadzoneShape>("DeadzoneShape").expect();
        bind_enum_value("Ellipse", input::DeadzoneShape::Ellipse).expect();
        bind_enum_value("Quad", input::DeadzoneShape::Quad).expect();
        bind_enum_value("Cross", input::DeadzoneShape::Cross).expect();

        bind_type<input::Controller>("Controller").expect();
        bind_member_instance_function("get_name", &input::Controller::get_name).expect();
        bind_member_instance_function("has_gamepad", &input::Controller::has_gamepad).expect();

        bind_member_instance_function("get_deadzone_radius", &input::Controller::get_deadzone_radius).expect();
        bind_member_instance_function("set_deadzone_radius", &input::Controller::set_deadzone_radius).expect();
        bind_member_instance_function("clear_deadzone_radius", &input::Controller::clear_deadzone_radius).expect();
        bind_member_instance_function("get_deadzone_shape", &input::Controller::get_deadzone_shape).expect();
        bind_member_instance_function("set_deadzone_shape", &input::Controller::set_deadzone_shape).expect();
        bind_member_instance_function("clear_deadzone_shape", &input::Controller::clear_deadzone_shape).expect();
        bind_member_instance_function("get_axis_deadzone_radius", &input::Controller::get_axis_deadzone_radius).expect();
        bind_member_instance_function("set_axis_deadzone_radius", &input::Controller::set_axis_deadzone_radius).expect();
        bind_member_instance_function("clear_axis_deadzone_radius", &input::Controller::clear_axis_deadzone_radius).expect();
        bind_member_instance_function("get_axis_deadzone_shape", &input::Controller::get_axis_deadzone_shape).expect();
        bind_member_instance_function("set_axis_deadzone_shape", &input::Controller::set_axis_deadzone_shape).expect();
        bind_member_instance_function("clear_axis_deadzone_shape", &input::Controller::clear_axis_deadzone_shape).expect();

        bind_member_instance_function("attach_gamepad", &input::Controller::attach_gamepad).expect();
        bind_member_instance_function("attach_first_available_gamepad",
                &input::Controller::attach_first_available_gamepad).expect();
        bind_member_instance_function("detach_gamepad", &input::Controller::detach_gamepad).expect();

        bind_member_instance_function("bind_keyboard_key", &input::Controller::bind_keyboard_key).expect();
        bind_member_instance_function<void (input::Controller::*)(input::KeyboardScancode)>(
                "unbind_keyboard_key", &input::Controller::unbind_keyboard_key).expect();
        bind_member_instance_function<void (input::Controller::*)(input::KeyboardScancode, const std::string &)>(
                "unbind_keyboard_key_action", &input::Controller::unbind_keyboard_key).expect();

        bind_member_instance_function("bind_mouse_button", &input::Controller::bind_mouse_button).expect();
        bind_member_instance_function<void (input::Controller::*)(input::MouseButton)>(
                "unbind_mouse_button", &input::Controller::unbind_mouse_button).expect();
        bind_member_instance_function<void (input::Controller::*)(input::MouseButton, const std::string &)>(
                "unbind_mouse_button_action", &input::Controller::unbind_mouse_button).expect();

        bind_member_instance_function("bind_mouse_axis", &input::Controller::bind_mouse_axis).expect();
        bind_member_instance_function<void (input::Controller::*)(input::MouseAxis)>(
                "unbind_mouse_axis", &input::Controller::unbind_mouse_axis).expect();
        bind_member_instance_function<void (input::Controller::*)(input::MouseAxis, const std::string &)>(
                "unbind_mouse_axis_action", &input::Controller::unbind_mouse_axis).expect();

        bind_member_instance_function("bind_gamepad_button", &input::Controller::bind_gamepad_button).expect();
        bind_member_instance_function<void (input::Controller::*)(input::GamepadButton)>(
                "unbind_gamepad_button", &input::Controller::unbind_gamepad_button).expect();
        bind_member_instance_function<void (input::Controller::*)(input::GamepadButton, const std::string &)>(
                "unbind_gamepad_button_action", &input::Controller::unbind_gamepad_button).expect();

        bind_member_instance_function("bind_gamepad_axis", &input::Controller::bind_gamepad_axis).expect();
        bind_member_instance_function<void (input::Controller::*)(input::GamepadAxis)>(
                "unbind_gamepad_axis", &input::Controller::unbind_gamepad_axis).expect();
        bind_member_instance_function<void (input::Controller::*)(input::GamepadAxis, const std::string &)>(
                "unbind_gamepad_axis_action", &input::Controller::unbind_gamepad_axis).expect();

        bind_member_instance_function("get_gamepad_name", &input::Controller::get_gamepad_name).expect();
        bind_member_instance_function("is_gamepad_button_pressed", &input::Controller::is_gamepad_button_pressed).expect();
        bind_member_instance_function("get_gamepad_axis", &input::Controller::get_gamepad_axis).expect();
        bind_member_instance_function("get_gamepad_axis_delta", &input::Controller::get_gamepad_axis_delta).expect();

        bind_member_instance_function("is_action_pressed", &input::Controller::is_action_pressed).expect();
        bind_member_instance_function("get_action_axis", &input::Controller::get_action_axis).expect();
        bind_member_instance_function("get_action_axis_delta", &input::Controller::get_action_axis_delta).expect();
    }

    static void _bind_event_symbols(void) {
        bind_enum<input::InputEventType>("InputEventType").expect();
        bind_enum_value("ButtonDown", input::InputEventType::ButtonDown).expect();
        bind_enum_value("ButtonUp", input::InputEventType::ButtonUp).expect();
        bind_enum_value("AxisChanged", input::InputEventType::AxisChanged).expect();

        bind_type<input::InputEvent>("InputEvent").expect();
        bind_member_field("input_type", &input::InputEvent::input_type).expect();
        bind_member_field("controller_name", &input::InputEvent::controller_name).expect();
        bind_member_field("action", &input::InputEvent::action).expect();
        bind_member_field("axis_value", &input::InputEvent::axis_value).expect();
        bind_member_field("axis_delta", &input::InputEvent::axis_delta).expect();
        bind_member_instance_function("get_window", &input::InputEvent::get_window).expect();

        bind_global_function(
                "register_input_handler",
                +[](std::function<void(const input::InputEvent &)> fn, Ordering ordering) -> Index {
                    return register_event_handler<input::InputEvent>(std::move(fn), TargetThread::Update, ordering);
                }
        ).expect();

        bind_enum<input::InputDeviceEventType>("InputDeviceEventType").expect();
        bind_enum_value("GamepadConnected", input::InputDeviceEventType::GamepadConnected).expect();
        bind_enum_value("GamepadDisconnected", input::InputDeviceEventType::GamepadDisconnected).expect();

        bind_type<input::InputDeviceEvent>("InputDeviceEvent").expect();
        bind_member_field("device_event", &input::InputDeviceEvent::device_event).expect();
        bind_member_field("controller_name", &input::InputDeviceEvent::controller_name).expect();
        bind_member_field("device_id", &input::InputDeviceEvent::device_id).expect();

        bind_global_function(
                "register_input_device_event_handler",
                +[](std::function<void(const input::InputDeviceEvent &)> fn, Ordering ordering) -> Index {
                    return register_event_handler<input::InputDeviceEvent>(std::move(fn),
                            TargetThread::Update, ordering);
                }
        ).expect();
    }

    void register_input_script_bindings(void) {
        _bind_input_manager_symbols();
        _bind_keyboard_symbols();
        _bind_mouse_symbols();
        _bind_gamepad_symbols();
        _bind_controller_symbols();
        _bind_event_symbols();
    }
}
