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

#include "argus/input.hpp"
#include "internal/input/script_bindings.hpp"

namespace argus {
    static void _bind_input_manager_symbols(void) {
        bind_type<input::InputManager>("InputManager");
        bind_member_instance_function("get_controller", &input::InputManager::get_controller);
        bind_member_instance_function("add_controller", &input::InputManager::add_controller);
        bind_extension_function<input::InputManager>("add_kbm_controller",
                +[](input::InputManager &manager, const std::string &name) -> input::Controller & {
                    return manager.add_controller(name, false);
                });
        bind_extension_function<input::InputManager>("add_gamepad_controller",
                +[](input::InputManager &manager, const std::string &name) -> input::Controller & {
                    return manager.add_controller(name, true);
            });
        bind_member_instance_function<void(input::InputManager::*)(const std::string &)>("remove_controller",
                &input::InputManager::remove_controller);

        bind_global_function("get_input_manager", &input::InputManager::instance);
    }

    static void _bind_keyboard_symbols(void) {
        bind_enum<input::KeyboardScancode>("KeyboardScancode");
        bind_enum_value("Unknown", input::KeyboardScancode::Unknown);
        bind_enum_value("A", input::KeyboardScancode::A);
        bind_enum_value("B", input::KeyboardScancode::B);
        bind_enum_value("C", input::KeyboardScancode::C);
        bind_enum_value("D", input::KeyboardScancode::D);
        bind_enum_value("E", input::KeyboardScancode::E);
        bind_enum_value("F", input::KeyboardScancode::F);
        bind_enum_value("G", input::KeyboardScancode::G);
        bind_enum_value("H", input::KeyboardScancode::H);
        bind_enum_value("I", input::KeyboardScancode::I);
        bind_enum_value("J", input::KeyboardScancode::J);
        bind_enum_value("K", input::KeyboardScancode::K);
        bind_enum_value("L", input::KeyboardScancode::L);
        bind_enum_value("M", input::KeyboardScancode::M);
        bind_enum_value("N", input::KeyboardScancode::N);
        bind_enum_value("O", input::KeyboardScancode::O);
        bind_enum_value("P", input::KeyboardScancode::P);
        bind_enum_value("Q", input::KeyboardScancode::Q);
        bind_enum_value("R", input::KeyboardScancode::R);
        bind_enum_value("S", input::KeyboardScancode::S);
        bind_enum_value("T", input::KeyboardScancode::T);
        bind_enum_value("U", input::KeyboardScancode::U);
        bind_enum_value("V", input::KeyboardScancode::V);
        bind_enum_value("W", input::KeyboardScancode::W);
        bind_enum_value("X", input::KeyboardScancode::X);
        bind_enum_value("Y", input::KeyboardScancode::Y);
        bind_enum_value("Z", input::KeyboardScancode::Z);
        bind_enum_value("Number1", input::KeyboardScancode::Number1);
        bind_enum_value("Number2", input::KeyboardScancode::Number2);
        bind_enum_value("Number3", input::KeyboardScancode::Number3);
        bind_enum_value("Number4", input::KeyboardScancode::Number4);
        bind_enum_value("Number5", input::KeyboardScancode::Number5);
        bind_enum_value("Number6", input::KeyboardScancode::Number6);
        bind_enum_value("Number7", input::KeyboardScancode::Number7);
        bind_enum_value("Number8", input::KeyboardScancode::Number8);
        bind_enum_value("Number9", input::KeyboardScancode::Number9);
        bind_enum_value("Number0", input::KeyboardScancode::Number0);
        bind_enum_value("Enter", input::KeyboardScancode::Enter);
        bind_enum_value("Escape", input::KeyboardScancode::Escape);
        bind_enum_value("Backspace", input::KeyboardScancode::Backspace);
        bind_enum_value("Tab", input::KeyboardScancode::Tab);
        bind_enum_value("Space", input::KeyboardScancode::Space);
        bind_enum_value("Minus", input::KeyboardScancode::Minus);
        bind_enum_value("Equals", input::KeyboardScancode::Equals);
        bind_enum_value("LeftBracket", input::KeyboardScancode::LeftBracket);
        bind_enum_value("RightBracket", input::KeyboardScancode::RightBracket);
        bind_enum_value("BackSlash", input::KeyboardScancode::BackSlash);
        bind_enum_value("Semicolon", input::KeyboardScancode::Semicolon);
        bind_enum_value("Apostrophe", input::KeyboardScancode::Apostrophe);
        bind_enum_value("Grave", input::KeyboardScancode::Grave);
        bind_enum_value("Comma", input::KeyboardScancode::Comma);
        bind_enum_value("Period", input::KeyboardScancode::Period);
        bind_enum_value("ForwardSlash", input::KeyboardScancode::ForwardSlash);
        bind_enum_value("CapsLock", input::KeyboardScancode::CapsLock);
        bind_enum_value("F1", input::KeyboardScancode::F1);
        bind_enum_value("F2", input::KeyboardScancode::F2);
        bind_enum_value("F3", input::KeyboardScancode::F3);
        bind_enum_value("F4", input::KeyboardScancode::F4);
        bind_enum_value("F6", input::KeyboardScancode::F6);
        bind_enum_value("F7", input::KeyboardScancode::F7);
        bind_enum_value("F8", input::KeyboardScancode::F8);
        bind_enum_value("F5", input::KeyboardScancode::F5);
        bind_enum_value("F9", input::KeyboardScancode::F9);
        bind_enum_value("F10", input::KeyboardScancode::F10);
        bind_enum_value("F11", input::KeyboardScancode::F11);
        bind_enum_value("F12", input::KeyboardScancode::F12);
        bind_enum_value("PrintScreen", input::KeyboardScancode::PrintScreen);
        bind_enum_value("ScrollLock", input::KeyboardScancode::ScrollLock);
        bind_enum_value("Pause", input::KeyboardScancode::Pause);
        bind_enum_value("Insert", input::KeyboardScancode::Insert);
        bind_enum_value("Home", input::KeyboardScancode::Home);
        bind_enum_value("PageUp", input::KeyboardScancode::PageUp);
        bind_enum_value("Delete", input::KeyboardScancode::Delete);
        bind_enum_value("End", input::KeyboardScancode::End);
        bind_enum_value("PageDown", input::KeyboardScancode::PageDown);
        bind_enum_value("ArrowRight", input::KeyboardScancode::ArrowRight);
        bind_enum_value("ArrowLeft", input::KeyboardScancode::ArrowLeft);
        bind_enum_value("ArrowDown", input::KeyboardScancode::ArrowDown);
        bind_enum_value("ArrowUp", input::KeyboardScancode::ArrowUp);
        bind_enum_value("NumpadNumLock", input::KeyboardScancode::NumpadNumLock);
        bind_enum_value("NumpadDivide", input::KeyboardScancode::NumpadDivide);
        bind_enum_value("NumpadTimes", input::KeyboardScancode::NumpadTimes);
        bind_enum_value("NumpadMinus", input::KeyboardScancode::NumpadMinus);
        bind_enum_value("NumpadPlus", input::KeyboardScancode::NumpadPlus);
        bind_enum_value("NumpadEnter", input::KeyboardScancode::NumpadEnter);
        bind_enum_value("Numpad1", input::KeyboardScancode::Numpad1);
        bind_enum_value("Numpad2", input::KeyboardScancode::Numpad2);
        bind_enum_value("Numpad3", input::KeyboardScancode::Numpad3);
        bind_enum_value("Numpad4", input::KeyboardScancode::Numpad4);
        bind_enum_value("Numpad5", input::KeyboardScancode::Numpad5);
        bind_enum_value("Numpad6", input::KeyboardScancode::Numpad6);
        bind_enum_value("Numpad7", input::KeyboardScancode::Numpad7);
        bind_enum_value("Numpad8", input::KeyboardScancode::Numpad8);
        bind_enum_value("Numpad9", input::KeyboardScancode::Numpad9);
        bind_enum_value("Numpad0", input::KeyboardScancode::Numpad0);
        bind_enum_value("NumpadDot", input::KeyboardScancode::NumpadDot);
        bind_enum_value("NumpadEquals", input::KeyboardScancode::NumpadEquals);
        bind_enum_value("Menu", input::KeyboardScancode::Menu);
        bind_enum_value("LeftControl", input::KeyboardScancode::LeftControl);
        bind_enum_value("LeftShift", input::KeyboardScancode::LeftShift);
        bind_enum_value("LeftAlt", input::KeyboardScancode::LeftAlt);
        bind_enum_value("Super", input::KeyboardScancode::Super);
        bind_enum_value("RightControl", input::KeyboardScancode::RightControl);
        bind_enum_value("RightShift", input::KeyboardScancode::RightShift);
        bind_enum_value("RightAlt", input::KeyboardScancode::RightAlt);

        bind_enum<input::KeyboardCommand>("KeyboardCommand");
        bind_enum_value("Escape", input::KeyboardCommand::Escape);
        bind_enum_value("F1", input::KeyboardCommand::F1);
        bind_enum_value("F2", input::KeyboardCommand::F2);
        bind_enum_value("F3", input::KeyboardCommand::F3);
        bind_enum_value("F4", input::KeyboardCommand::F4);
        bind_enum_value("F5", input::KeyboardCommand::F5);
        bind_enum_value("F6", input::KeyboardCommand::F6);
        bind_enum_value("F7", input::KeyboardCommand::F7);
        bind_enum_value("F8", input::KeyboardCommand::F8);
        bind_enum_value("F9", input::KeyboardCommand::F9);
        bind_enum_value("F10", input::KeyboardCommand::F10);
        bind_enum_value("F11", input::KeyboardCommand::F11);
        bind_enum_value("F12", input::KeyboardCommand::F12);
        bind_enum_value("Backspace", input::KeyboardCommand::Backspace);
        bind_enum_value("Tab", input::KeyboardCommand::Tab);
        bind_enum_value("CapsLock", input::KeyboardCommand::CapsLock);
        bind_enum_value("Enter", input::KeyboardCommand::Enter);
        bind_enum_value("Menu", input::KeyboardCommand::Menu);
        bind_enum_value("PrintScreen", input::KeyboardCommand::PrintScreen);
        bind_enum_value("ScrollLock", input::KeyboardCommand::ScrollLock);
        bind_enum_value("Break", input::KeyboardCommand::Break);
        bind_enum_value("Insert", input::KeyboardCommand::Insert);
        bind_enum_value("Home", input::KeyboardCommand::Home);
        bind_enum_value("PageUp", input::KeyboardCommand::PageUp);
        bind_enum_value("Delete", input::KeyboardCommand::Delete);
        bind_enum_value("End", input::KeyboardCommand::End);
        bind_enum_value("PageDown", input::KeyboardCommand::PageDown);
        bind_enum_value("ArrowUp", input::KeyboardCommand::ArrowUp);
        bind_enum_value("ArrowLeft", input::KeyboardCommand::ArrowLeft);
        bind_enum_value("ArrowDown", input::KeyboardCommand::ArrowDown);
        bind_enum_value("ArrowRight", input::KeyboardCommand::ArrowRight);
        bind_enum_value("NumpadNumLock", input::KeyboardCommand::NumpadNumLock);
        bind_enum_value("NumpadEnter", input::KeyboardCommand::NumpadEnter);
        bind_enum_value("NumpadDot", input::KeyboardCommand::NumpadDot);
        bind_enum_value("Super", input::KeyboardCommand::Super);

        bind_enum<input::KeyboardModifiers>("KeyboardModifiers");
        bind_enum_value("None", input::KeyboardModifiers::None);
        bind_enum_value("Shift", input::KeyboardModifiers::Shift);
        bind_enum_value("Control", input::KeyboardModifiers::Control);
        bind_enum_value("Super", input::KeyboardModifiers::Super);
        bind_enum_value("Alt", input::KeyboardModifiers::Alt);

        bind_global_function("get_key_name", input::get_key_name);
        bind_global_function("is_key_pressed", input::is_key_pressed);
    }

    static void _bind_mouse_symbols(void) {
        bind_enum<input::MouseButton>("MouseButton");
        bind_enum_value("Primary", input::MouseButton::Primary);
        bind_enum_value("Secondary", input::MouseButton::Secondary);
        bind_enum_value("Middle", input::MouseButton::Middle);
        bind_enum_value("Back", input::MouseButton::Back);
        bind_enum_value("Forward", input::MouseButton::Forward);

        bind_enum<input::MouseAxis>("MouseAxis");
        bind_enum_value("Horizontal", input::MouseAxis::Horizontal);
        bind_enum_value("Vertical", input::MouseAxis::Vertical);

        bind_global_function("mouse_delta", input::mouse_delta);
        bind_global_function("mouse_pos", input::mouse_pos);
    }

    static void _bind_gamepad_symbols(void) {
        bind_enum<input::GamepadButton>("GamepadButton");
        bind_enum_value("Unknown", input::GamepadButton::Unknown);
        bind_enum_value("A", input::GamepadButton::A);
        bind_enum_value("B", input::GamepadButton::B);
        bind_enum_value("X", input::GamepadButton::X);
        bind_enum_value("Y", input::GamepadButton::Y);
        bind_enum_value("DpadUp", input::GamepadButton::DpadUp);
        bind_enum_value("DpadDown", input::GamepadButton::DpadDown);
        bind_enum_value("DpadLeft", input::GamepadButton::DpadLeft);
        bind_enum_value("DpadRight", input::GamepadButton::DpadRight);
        bind_enum_value("LBumper", input::GamepadButton::LBumper);
        bind_enum_value("RBumper", input::GamepadButton::RBumper);
        bind_enum_value("LTrigger", input::GamepadButton::LTrigger);
        bind_enum_value("RTrigger", input::GamepadButton::RTrigger);
        bind_enum_value("LStick", input::GamepadButton::LStick);
        bind_enum_value("RStick", input::GamepadButton::RStick);
        bind_enum_value("L4", input::GamepadButton::L4);
        bind_enum_value("R4", input::GamepadButton::R4);
        bind_enum_value("L5", input::GamepadButton::L5);
        bind_enum_value("R5", input::GamepadButton::R5);
        bind_enum_value("Start", input::GamepadButton::Start);
        bind_enum_value("Back", input::GamepadButton::Back);
        bind_enum_value("Guide", input::GamepadButton::Guide);
        bind_enum_value("Misc1", input::GamepadButton::Misc1);
        bind_enum_value("MaxValue", input::GamepadButton::MaxValue);

        bind_enum<input::GamepadAxis>("GamepadAxis");
        bind_enum_value("Unknown", input::GamepadAxis::Unknown);
        bind_enum_value("LeftX", input::GamepadAxis::LeftX);
        bind_enum_value("LeftY", input::GamepadAxis::LeftY);
        bind_enum_value("RightX", input::GamepadAxis::RightX);
        bind_enum_value("RightY", input::GamepadAxis::RightY);
        bind_enum_value("LTrigger", input::GamepadAxis::LTrigger);
        bind_enum_value("RTrigger", input::GamepadAxis::RTrigger);
        bind_enum_value("MaxValue", input::GamepadAxis::MaxValue);

        bind_global_function("get_gamepad_name", input::get_gamepad_name);
        bind_global_function("is_gamepad_button_pressed", input::is_gamepad_button_pressed);
        bind_global_function("get_gamepad_axis", input::get_gamepad_axis);
    }

    static void _bind_controller_symbols(void) {
        bind_type<input::Controller>("Controller");
        bind_member_instance_function("get_name", &input::Controller::get_name);
        bind_member_instance_function("has_gamepad", &input::Controller::has_gamepad);

        bind_member_instance_function("bind_keyboard_key", &input::Controller::bind_keyboard_key);
        bind_member_instance_function<void(input::Controller::*)(input::KeyboardScancode)>(
                "unbind_keyboard_key", &input::Controller::unbind_keyboard_key);
        bind_member_instance_function<void(input::Controller::*)(input::KeyboardScancode, const std::string &)>(
                "unbind_keyboard_key_action", &input::Controller::unbind_keyboard_key);

        bind_member_instance_function("bind_mouse_button", &input::Controller::bind_mouse_button);
        bind_member_instance_function<void(input::Controller::*)(input::MouseButton)>(
                "unbind_mouse_button", &input::Controller::unbind_mouse_button);
        bind_member_instance_function<void(input::Controller::*)(input::MouseButton, const std::string &)>(
                "unbind_mouse_button_action", &input::Controller::unbind_mouse_button);

        bind_member_instance_function("bind_mouse_axis", &input::Controller::bind_mouse_axis);
        bind_member_instance_function<void(input::Controller::*)(input::MouseAxis)>(
                "unbind_mouse_axis", &input::Controller::unbind_mouse_axis);
        bind_member_instance_function<void(input::Controller::*)(input::MouseAxis, const std::string &)>(
                "unbind_mouse_axis_action", &input::Controller::unbind_mouse_axis);

        bind_member_instance_function("bind_gamepad_button", &input::Controller::bind_gamepad_button);
        bind_member_instance_function<void(input::Controller::*)(input::GamepadButton)>(
                "unbind_gamepad_button", &input::Controller::unbind_gamepad_button);
        bind_member_instance_function<void(input::Controller::*)(input::GamepadButton, const std::string &)>(
                "unbind_gamepad_button_action", &input::Controller::unbind_gamepad_button);

        bind_member_instance_function("bind_gamepad_axis", &input::Controller::bind_gamepad_axis);
        bind_member_instance_function<void(input::Controller::*)(input::GamepadAxis)>(
                "unbind_gamepad_axis", &input::Controller::unbind_gamepad_axis);
        bind_member_instance_function<void(input::Controller::*)(input::GamepadAxis, const std::string &)>(
                "unbind_gamepad_axis_action", &input::Controller::unbind_gamepad_axis);

        bind_member_instance_function("get_gamepad_name", &input::Controller::get_gamepad_name);
        bind_member_instance_function("is_gamepad_button_pressed", &input::Controller::is_gamepad_button_pressed);
        bind_member_instance_function("get_gamepad_axis", &input::Controller::get_gamepad_axis);

        bind_member_instance_function("is_action_pressed", &input::Controller::is_action_pressed);
        bind_member_instance_function("get_action_axis", &input::Controller::get_action_axis);
        bind_member_instance_function("get_action_axis_delta", &input::Controller::get_action_axis_delta);
    }

    static void _bind_event_symbols(void) {
        bind_enum<input::InputEventType>("InputEventType");
        bind_enum_value("ButtonDown", input::InputEventType::ButtonDown);
        bind_enum_value("ButtonUp", input::InputEventType::ButtonUp);
        bind_enum_value("AxisChanged", input::InputEventType::AxisChanged);

        bind_type<input::InputEvent>("InputEvent");
        bind_member_field("input_type", &input::InputEvent::input_type);
        bind_member_field("controller_name", &input::InputEvent::controller_name);
        bind_member_field("action", &input::InputEvent::action);
        bind_member_field("axis_value", &input::InputEvent::axis_value);
        bind_member_field("axis_delta", &input::InputEvent::axis_delta);
        bind_member_instance_function("get_window", &input::InputEvent::get_window);

        bind_global_function(
                "register_input_handler",
                +[](std::function<void(const input::InputEvent &)> fn, Ordering ordering) -> Index {
                    return register_event_handler<input::InputEvent>(std::move(fn), TargetThread::Update, ordering);
                }
        );
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
