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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/vector.hpp"

#include "argus/core/event.hpp"

#include "argus/wm/window.hpp"

#include "argus/input/input_manager.hpp"
#include "argus/input/keyboard.hpp"
#include "internal/input/keyboard.hpp"

#include "GLFW/glfw3.h"

#include <string>
#include <type_traits>
#include <unordered_map>

namespace argus::input {
    static const std::unordered_map<int, KeyboardScancode> g_keycode_glfw_to_argus({
            {GLFW_KEY_SPACE,         KeyboardScancode::Space},
            {GLFW_KEY_APOSTROPHE,    KeyboardScancode::Apostrophe},
            {GLFW_KEY_COMMA,         KeyboardScancode::Comma},
            {GLFW_KEY_MINUS,         KeyboardScancode::Minus},
            {GLFW_KEY_PERIOD,        KeyboardScancode::Period},
            {GLFW_KEY_SLASH,         KeyboardScancode::ForwardSlash},
            {GLFW_KEY_0,             KeyboardScancode::Number0},
            {GLFW_KEY_1,             KeyboardScancode::Number1},
            {GLFW_KEY_2,             KeyboardScancode::Number2},
            {GLFW_KEY_3,             KeyboardScancode::Number3},
            {GLFW_KEY_4,             KeyboardScancode::Number4},
            {GLFW_KEY_5,             KeyboardScancode::Number5},
            {GLFW_KEY_6,             KeyboardScancode::Number6},
            {GLFW_KEY_7,             KeyboardScancode::Number7},
            {GLFW_KEY_8,             KeyboardScancode::Number8},
            {GLFW_KEY_9,             KeyboardScancode::Number9},
            {GLFW_KEY_SEMICOLON,     KeyboardScancode::Semicolon},
            {GLFW_KEY_EQUAL,         KeyboardScancode::Equals},
            {GLFW_KEY_LEFT_BRACKET,  KeyboardScancode::LeftBracket},
            {GLFW_KEY_BACKSLASH,     KeyboardScancode::BackSlash},
            {GLFW_KEY_RIGHT_BRACKET, KeyboardScancode::RightBracket},
            {GLFW_KEY_GRAVE_ACCENT,  KeyboardScancode::Grave},
            {GLFW_KEY_ESCAPE,        KeyboardScancode::Escape},
            {GLFW_KEY_ENTER,         KeyboardScancode::Enter},
            {GLFW_KEY_TAB,           KeyboardScancode::Tab},
            {GLFW_KEY_BACKSPACE,     KeyboardScancode::Backspace},
            {GLFW_KEY_INSERT,        KeyboardScancode::Insert},
            {GLFW_KEY_DELETE,        KeyboardScancode::Delete},
            {GLFW_KEY_RIGHT,         KeyboardScancode::ArrowRight},
            {GLFW_KEY_LEFT,          KeyboardScancode::ArrowLeft},
            {GLFW_KEY_DOWN,          KeyboardScancode::ArrowDown},
            {GLFW_KEY_UP,            KeyboardScancode::ArrowUp},
            {GLFW_KEY_PAGE_UP,       KeyboardScancode::PageUp},
            {GLFW_KEY_PAGE_DOWN,     KeyboardScancode::PageDown},
            {GLFW_KEY_HOME,          KeyboardScancode::Home},
            {GLFW_KEY_END,           KeyboardScancode::End},
            {GLFW_KEY_CAPS_LOCK,     KeyboardScancode::CapsLock},
            {GLFW_KEY_SCROLL_LOCK,   KeyboardScancode::ScrollLock},
            {GLFW_KEY_NUM_LOCK,      KeyboardScancode::NumpadNumLock},
            {GLFW_KEY_PRINT_SCREEN,  KeyboardScancode::PrintScreen},
            {GLFW_KEY_PAUSE,         KeyboardScancode::Pause},
            {GLFW_KEY_F1,            KeyboardScancode::F1},
            {GLFW_KEY_F2,            KeyboardScancode::F2},
            {GLFW_KEY_F3,            KeyboardScancode::F3},
            {GLFW_KEY_F4,            KeyboardScancode::F4},
            {GLFW_KEY_F5,            KeyboardScancode::F5},
            {GLFW_KEY_F6,            KeyboardScancode::F6},
            {GLFW_KEY_F7,            KeyboardScancode::F7},
            {GLFW_KEY_F8,            KeyboardScancode::F8},
            {GLFW_KEY_F9,            KeyboardScancode::F9},
            {GLFW_KEY_F10,           KeyboardScancode::F10},
            {GLFW_KEY_F11,           KeyboardScancode::F11},
            {GLFW_KEY_F12,           KeyboardScancode::F12},
            {GLFW_KEY_KP_0,          KeyboardScancode::Numpad0},
            {GLFW_KEY_KP_1,          KeyboardScancode::Numpad1},
            {GLFW_KEY_KP_2,          KeyboardScancode::Numpad2},
            {GLFW_KEY_KP_3,          KeyboardScancode::Numpad3},
            {GLFW_KEY_KP_4,          KeyboardScancode::Numpad4},
            {GLFW_KEY_KP_5,          KeyboardScancode::Numpad5},
            {GLFW_KEY_KP_6,          KeyboardScancode::Numpad6},
            {GLFW_KEY_KP_7,          KeyboardScancode::Numpad7},
            {GLFW_KEY_KP_8,          KeyboardScancode::Numpad8},
            {GLFW_KEY_KP_9,          KeyboardScancode::Numpad9},
            {GLFW_KEY_KP_DECIMAL,    KeyboardScancode::NumpadDot},
            {GLFW_KEY_KP_DIVIDE,     KeyboardScancode::NumpadDivide},
            {GLFW_KEY_KP_MULTIPLY,   KeyboardScancode::NumpadTimes},
            {GLFW_KEY_KP_SUBTRACT,   KeyboardScancode::NumpadMinus},
            {GLFW_KEY_KP_ADD,        KeyboardScancode::NumpadPlus},
            {GLFW_KEY_KP_ENTER,      KeyboardScancode::NumpadEnter},
            {GLFW_KEY_KP_EQUAL,      KeyboardScancode::NumpadEquals},
            {GLFW_KEY_LEFT_SHIFT,    KeyboardScancode::LeftShift},
            {GLFW_KEY_LEFT_CONTROL,  KeyboardScancode::LeftControl},
            {GLFW_KEY_LEFT_ALT,      KeyboardScancode::LeftAlt},
            {GLFW_KEY_LEFT_SUPER,    KeyboardScancode::Super},
            {GLFW_KEY_RIGHT_SHIFT,   KeyboardScancode::RightShift},
            {GLFW_KEY_RIGHT_CONTROL, KeyboardScancode::RightControl},
            {GLFW_KEY_RIGHT_ALT,     KeyboardScancode::RightAlt},
            {GLFW_KEY_RIGHT_SUPER,   KeyboardScancode::Super},
            {GLFW_KEY_MENU,          KeyboardScancode::Menu},
    });
    static const std::unordered_map<KeyboardScancode, int> g_keycode_argus_to_glfw({
            {KeyboardScancode::Space,         GLFW_KEY_SPACE},
            {KeyboardScancode::Apostrophe,    GLFW_KEY_APOSTROPHE},
            {KeyboardScancode::Comma,         GLFW_KEY_COMMA},
            {KeyboardScancode::Minus,         GLFW_KEY_MINUS},
            {KeyboardScancode::Period,        GLFW_KEY_PERIOD},
            {KeyboardScancode::ForwardSlash,  GLFW_KEY_SLASH},
            {KeyboardScancode::Number0,       GLFW_KEY_0},
            {KeyboardScancode::Number1,       GLFW_KEY_1},
            {KeyboardScancode::Number2,       GLFW_KEY_2},
            {KeyboardScancode::Number3,       GLFW_KEY_3},
            {KeyboardScancode::Number4,       GLFW_KEY_4},
            {KeyboardScancode::Number5,       GLFW_KEY_5},
            {KeyboardScancode::Number6,       GLFW_KEY_6},
            {KeyboardScancode::Number7,       GLFW_KEY_7},
            {KeyboardScancode::Number8,       GLFW_KEY_8},
            {KeyboardScancode::Number9,       GLFW_KEY_9},
            {KeyboardScancode::Semicolon,     GLFW_KEY_SEMICOLON},
            {KeyboardScancode::Equals,        GLFW_KEY_EQUAL},
            {KeyboardScancode::LeftBracket,   GLFW_KEY_LEFT_BRACKET},
            {KeyboardScancode::BackSlash,     GLFW_KEY_BACKSLASH},
            {KeyboardScancode::RightBracket,  GLFW_KEY_RIGHT_BRACKET},
            {KeyboardScancode::Grave,         GLFW_KEY_GRAVE_ACCENT},
            {KeyboardScancode::Escape,        GLFW_KEY_ESCAPE},
            {KeyboardScancode::Enter,         GLFW_KEY_ENTER},
            {KeyboardScancode::Tab,           GLFW_KEY_TAB},
            {KeyboardScancode::Backspace,     GLFW_KEY_BACKSPACE},
            {KeyboardScancode::Insert,        GLFW_KEY_INSERT},
            {KeyboardScancode::Delete,        GLFW_KEY_DELETE},
            {KeyboardScancode::ArrowRight,    GLFW_KEY_RIGHT},
            {KeyboardScancode::ArrowLeft,     GLFW_KEY_LEFT},
            {KeyboardScancode::ArrowDown,     GLFW_KEY_DOWN},
            {KeyboardScancode::ArrowUp,       GLFW_KEY_UP},
            {KeyboardScancode::PageUp,        GLFW_KEY_PAGE_UP},
            {KeyboardScancode::PageDown,      GLFW_KEY_PAGE_DOWN},
            {KeyboardScancode::Home,          GLFW_KEY_HOME},
            {KeyboardScancode::End,           GLFW_KEY_END},
            {KeyboardScancode::CapsLock,      GLFW_KEY_CAPS_LOCK},
            {KeyboardScancode::ScrollLock,    GLFW_KEY_SCROLL_LOCK},
            {KeyboardScancode::NumpadNumLock, GLFW_KEY_NUM_LOCK},
            {KeyboardScancode::PrintScreen,   GLFW_KEY_PRINT_SCREEN},
            {KeyboardScancode::Pause,         GLFW_KEY_PAUSE},
            {KeyboardScancode::F1,            GLFW_KEY_F1},
            {KeyboardScancode::F2,            GLFW_KEY_F2},
            {KeyboardScancode::F3,            GLFW_KEY_F3},
            {KeyboardScancode::F4,            GLFW_KEY_F4},
            {KeyboardScancode::F5,            GLFW_KEY_F5},
            {KeyboardScancode::F6,            GLFW_KEY_F6},
            {KeyboardScancode::F7,            GLFW_KEY_F7},
            {KeyboardScancode::F8,            GLFW_KEY_F8},
            {KeyboardScancode::F9,            GLFW_KEY_F9},
            {KeyboardScancode::F10,           GLFW_KEY_F10},
            {KeyboardScancode::F11,           GLFW_KEY_F11},
            {KeyboardScancode::F12,           GLFW_KEY_F12},
            {KeyboardScancode::Numpad0,       GLFW_KEY_KP_0},
            {KeyboardScancode::Numpad1,       GLFW_KEY_KP_1},
            {KeyboardScancode::Numpad2,       GLFW_KEY_KP_2},
            {KeyboardScancode::Numpad3,       GLFW_KEY_KP_3},
            {KeyboardScancode::Numpad4,       GLFW_KEY_KP_4},
            {KeyboardScancode::Numpad5,       GLFW_KEY_KP_5},
            {KeyboardScancode::Numpad6,       GLFW_KEY_KP_6},
            {KeyboardScancode::Numpad7,       GLFW_KEY_KP_7},
            {KeyboardScancode::Numpad8,       GLFW_KEY_KP_8},
            {KeyboardScancode::Numpad9,       GLFW_KEY_KP_9},
            {KeyboardScancode::NumpadDot,     GLFW_KEY_KP_DECIMAL},
            {KeyboardScancode::NumpadDivide,  GLFW_KEY_KP_DIVIDE},
            {KeyboardScancode::NumpadTimes,   GLFW_KEY_KP_MULTIPLY},
            {KeyboardScancode::NumpadMinus,   GLFW_KEY_KP_SUBTRACT},
            {KeyboardScancode::NumpadPlus,    GLFW_KEY_KP_ADD},
            {KeyboardScancode::NumpadEnter,   GLFW_KEY_KP_ENTER},
            {KeyboardScancode::NumpadEquals,  GLFW_KEY_KP_EQUAL},
            {KeyboardScancode::LeftShift,     GLFW_KEY_LEFT_SHIFT},
            {KeyboardScancode::LeftControl,   GLFW_KEY_LEFT_CONTROL},
            {KeyboardScancode::LeftAlt,       GLFW_KEY_LEFT_ALT},
            {KeyboardScancode::Super,         GLFW_KEY_LEFT_SUPER},
            {KeyboardScancode::RightShift,    GLFW_KEY_RIGHT_SHIFT},
            {KeyboardScancode::RightControl,  GLFW_KEY_RIGHT_CONTROL},
            {KeyboardScancode::RightAlt,      GLFW_KEY_RIGHT_ALT},
            {KeyboardScancode::Menu,          GLFW_KEY_MENU},
    });

    /*static KeyboardModifiers _translate_glfw_keymod(uint16_t glfw_keymod) {
        KeyboardModifiers mod = KeyboardModifiers::None;

        if (glfw_keymod & GLFW_MOD_SHIFT) {
            mod |= KeyboardModifiers::Shift;
        }
        if (glfw_keymod & GLFW_MOD_CONTROL) {
            mod |= KeyboardModifiers::Control;
        }
        if (glfw_keymod & GLFW_MOD_SUPER) {
            mod |= KeyboardModifiers::Super;
        }
        if (glfw_keymod & GLFW_MOD_ALT) {
            mod |= KeyboardModifiers::Alt;
        }

        return mod;
    }*/

    static KeyboardScancode _translate_glfw_keycode(int glfw_keycode) {
        if (glfw_keycode < 0) {
            Logger::default_logger().warn("Encountered negative keycode %d", glfw_keycode);
            return KeyboardScancode::Unknown;
        }

        if (glfw_keycode >= GLFW_KEY_A && glfw_keycode <= GLFW_KEY_Z) {
            // GLFW shifts letter scancodes up by 61 to match ASCII
            return static_cast<KeyboardScancode>(glfw_keycode - 61);
        } else {
            auto res = g_keycode_glfw_to_argus.find(glfw_keycode);
            if (res == g_keycode_glfw_to_argus.end()) {
                Logger::default_logger().debug("Saw unknown GLFW scancode %d", glfw_keycode);
                return KeyboardScancode::Unknown;
            }
            return res->second;
        }
    }

    static int _translate_argus_keycode(KeyboardScancode argus_keycode) {
        if (argus_keycode >= KeyboardScancode::A && argus_keycode <= KeyboardScancode::Z) {
            // GLFW shifts letter scancodes up by 61 to match ASCII
            return int(argus_keycode) + 61;
        } else {
            auto res = g_keycode_argus_to_glfw.find(argus_keycode);
            if (res == g_keycode_argus_to_glfw.end()) {
                Logger::default_logger().warn("Saw unknown Argus scancode %d", int(argus_keycode));
                return GLFW_KEY_UNKNOWN;
            }
            return res->second;
        }
    }

    static void
    _on_key_event(GLFWwindow *glfw_window, int glfw_keycode, int glfw_scancode, int glfw_action, int glfw_mods) {
        UNUSED(glfw_scancode);
        UNUSED(glfw_mods);

        if (glfw_action != GLFW_PRESS && glfw_action != GLFW_RELEASE) {
            return;
        }

        auto *window = get_window_from_handle(glfw_window);
        if (window == nullptr) {
            return;
        }

        auto key = _translate_glfw_keycode(glfw_keycode);
        InputManager::instance().handle_key_press(*window, key, glfw_action == GLFW_RELEASE);
    }

    constexpr inline KeyboardModifiers operator|(const KeyboardModifiers lhs, const KeyboardModifiers rhs) {
        return static_cast<KeyboardModifiers>(static_cast<std::underlying_type<KeyboardModifiers>::type>(lhs)
                                              | static_cast<std::underlying_type<KeyboardModifiers>::type>(rhs));
    }

    inline KeyboardModifiers operator|=(KeyboardModifiers &lhs, const KeyboardModifiers rhs) {
        return lhs = static_cast<KeyboardModifiers>(static_cast<std::underlying_type<KeyboardModifiers>::type>(lhs)
                                                    | static_cast<std::underlying_type<KeyboardModifiers>::type>(rhs));
    }

    constexpr inline KeyboardModifiers operator&(const KeyboardModifiers lhs, const KeyboardModifiers rhs) {
        return static_cast<KeyboardModifiers>(static_cast<std::underlying_type<KeyboardModifiers>::type>(lhs)
                                              & static_cast<std::underlying_type<KeyboardModifiers>::type>(rhs));
    }

    void init_keyboard(const Window &window) {
        auto glfw_handle = static_cast<GLFWwindow *>(argus::get_window_handle(window));
        glfwSetKeyCallback(glfw_handle, _on_key_event);
    }

    std::string get_key_name(const KeyboardScancode scancode) {
        return glfwGetKeyName(0, _translate_argus_keycode(scancode));
    }

    bool keyboard_key_down(const argus::Window &window, const KeyboardScancode scancode) {
        int glfw_scancode = _translate_argus_keycode(scancode);
        if (glfw_scancode == GLFW_KEY_UNKNOWN) {
            return false;
        }
        return glfwGetKey(static_cast<GLFWwindow *>(argus::get_window_handle(window)), glfw_scancode);
    }
}
