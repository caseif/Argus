/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/event.hpp"
#include "internal/core/core_util.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "internal/wm/window.hpp"

// module input
#include "argus/input/keyboard.hpp"
#include "internal/input/keyboard.hpp"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cstdint>

namespace argus {
    static const std::unordered_map<int, KeyboardScancode> g_keycode_glfw_to_argus({
        {GLFW_KEY_SPACE, KeyboardScancode::SPACE},
        {GLFW_KEY_APOSTROPHE, KeyboardScancode::APOSTROPHE},
        {GLFW_KEY_COMMA, KeyboardScancode::COMMA},
        {GLFW_KEY_MINUS, KeyboardScancode::MINUS},
        {GLFW_KEY_PERIOD, KeyboardScancode::PERIOD},
        {GLFW_KEY_SLASH, KeyboardScancode::FORWARD_SLASH},
        {GLFW_KEY_0, KeyboardScancode::NUMBER_0},
        {GLFW_KEY_1, KeyboardScancode::NUMBER_1},
        {GLFW_KEY_2, KeyboardScancode::NUMBER_2},
        {GLFW_KEY_3, KeyboardScancode::NUMBER_3},
        {GLFW_KEY_4, KeyboardScancode::NUMBER_4},
        {GLFW_KEY_5, KeyboardScancode::NUMBER_5},
        {GLFW_KEY_6, KeyboardScancode::NUMBER_6},
        {GLFW_KEY_7, KeyboardScancode::NUMBER_7},
        {GLFW_KEY_8, KeyboardScancode::NUMBER_8},
        {GLFW_KEY_9, KeyboardScancode::NUMBER_9},
        {GLFW_KEY_SEMICOLON, KeyboardScancode::SEMICOLON},
        {GLFW_KEY_EQUAL, KeyboardScancode::EQUALS},
        {GLFW_KEY_LEFT_BRACKET, KeyboardScancode::LEFT_BRACKET},
        {GLFW_KEY_BACKSLASH, KeyboardScancode::BACK_SLASH},
        {GLFW_KEY_RIGHT_BRACKET, KeyboardScancode::RIGHT_BRACKET},
        {GLFW_KEY_GRAVE_ACCENT, KeyboardScancode::GRAVE},
        {GLFW_KEY_ESCAPE, KeyboardScancode::ESCAPE},
        {GLFW_KEY_ENTER, KeyboardScancode::ENTER},
        {GLFW_KEY_TAB, KeyboardScancode::TAB},
        {GLFW_KEY_BACKSPACE, KeyboardScancode::BACKSPACE},
        {GLFW_KEY_INSERT, KeyboardScancode::INSERT},
        {GLFW_KEY_DELETE, KeyboardScancode::DEL},
        {GLFW_KEY_RIGHT, KeyboardScancode::ARROW_RIGHT},
        {GLFW_KEY_LEFT, KeyboardScancode::ARROW_LEFT},
        {GLFW_KEY_DOWN, KeyboardScancode::ARROW_DOWN},
        {GLFW_KEY_UP, KeyboardScancode::ARROW_UP},
        {GLFW_KEY_PAGE_UP, KeyboardScancode::PAGE_UP},
        {GLFW_KEY_PAGE_DOWN, KeyboardScancode::PAGE_DOWN},
        {GLFW_KEY_HOME, KeyboardScancode::HOME},
        {GLFW_KEY_END, KeyboardScancode::END},
        {GLFW_KEY_CAPS_LOCK, KeyboardScancode::CAPS_LOCK},
        {GLFW_KEY_SCROLL_LOCK, KeyboardScancode::SCROLL_LOCK},
        {GLFW_KEY_NUM_LOCK, KeyboardScancode::NP_NUM_LOCK},
        {GLFW_KEY_PRINT_SCREEN, KeyboardScancode::PRINT_SCREEN},
        {GLFW_KEY_PAUSE, KeyboardScancode::PAUSE},
        {GLFW_KEY_F1, KeyboardScancode::F1},
        {GLFW_KEY_F2, KeyboardScancode::F2},
        {GLFW_KEY_F3, KeyboardScancode::F3},
        {GLFW_KEY_F4, KeyboardScancode::F4},
        {GLFW_KEY_F5, KeyboardScancode::F5},
        {GLFW_KEY_F6, KeyboardScancode::F6},
        {GLFW_KEY_F7, KeyboardScancode::F7},
        {GLFW_KEY_F8, KeyboardScancode::F8},
        {GLFW_KEY_F9, KeyboardScancode::F9},
        {GLFW_KEY_F10, KeyboardScancode::F10},
        {GLFW_KEY_F11, KeyboardScancode::F11},
        {GLFW_KEY_F12, KeyboardScancode::F12},
        {GLFW_KEY_KP_0, KeyboardScancode::NP_0},
        {GLFW_KEY_KP_1, KeyboardScancode::NP_1},
        {GLFW_KEY_KP_2, KeyboardScancode::NP_2},
        {GLFW_KEY_KP_3, KeyboardScancode::NP_3},
        {GLFW_KEY_KP_4, KeyboardScancode::NP_4},
        {GLFW_KEY_KP_5, KeyboardScancode::NP_5},
        {GLFW_KEY_KP_6, KeyboardScancode::NP_6},
        {GLFW_KEY_KP_7, KeyboardScancode::NP_7},
        {GLFW_KEY_KP_8, KeyboardScancode::NP_8},
        {GLFW_KEY_KP_9, KeyboardScancode::NP_9},
        {GLFW_KEY_KP_DECIMAL, KeyboardScancode::NP_DOT},
        {GLFW_KEY_KP_DIVIDE, KeyboardScancode::NP_DIVIDE},
        {GLFW_KEY_KP_MULTIPLY, KeyboardScancode::NP_TIMES},
        {GLFW_KEY_KP_SUBTRACT, KeyboardScancode::NP_MINUS},
        {GLFW_KEY_KP_ADD, KeyboardScancode::NP_PLUS},
        {GLFW_KEY_KP_ENTER, KeyboardScancode::NP_ENTER},
        {GLFW_KEY_KP_EQUAL, KeyboardScancode::NP_EQUALS},
        {GLFW_KEY_LEFT_SHIFT, KeyboardScancode::LEFT_SHIFT},
        {GLFW_KEY_LEFT_CONTROL, KeyboardScancode::LEFT_CONTROL},
        {GLFW_KEY_LEFT_ALT, KeyboardScancode::LEFT_ALT},
        {GLFW_KEY_LEFT_SUPER, KeyboardScancode::SUPER},
        {GLFW_KEY_RIGHT_SHIFT, KeyboardScancode::RIGHT_SHIFT},
        {GLFW_KEY_RIGHT_CONTROL, KeyboardScancode::RIGHT_CONTROL},
        {GLFW_KEY_RIGHT_ALT, KeyboardScancode::RIGHT_ALT},
        {GLFW_KEY_RIGHT_SUPER, KeyboardScancode::SUPER},
        {GLFW_KEY_MENU, KeyboardScancode::MENU},
    });
    static const std::unordered_map<KeyboardScancode, int> g_keycode_argus_to_glfw({
        {KeyboardScancode::SPACE, GLFW_KEY_SPACE},
        {KeyboardScancode::APOSTROPHE, GLFW_KEY_APOSTROPHE},
        {KeyboardScancode::COMMA, GLFW_KEY_COMMA},
        {KeyboardScancode::MINUS, GLFW_KEY_MINUS},
        {KeyboardScancode::PERIOD, GLFW_KEY_PERIOD},
        {KeyboardScancode::FORWARD_SLASH, GLFW_KEY_SLASH},
        {KeyboardScancode::NUMBER_0, GLFW_KEY_0},
        {KeyboardScancode::NUMBER_1, GLFW_KEY_1},
        {KeyboardScancode::NUMBER_2, GLFW_KEY_2},
        {KeyboardScancode::NUMBER_3, GLFW_KEY_3},
        {KeyboardScancode::NUMBER_4, GLFW_KEY_4},
        {KeyboardScancode::NUMBER_5, GLFW_KEY_5},
        {KeyboardScancode::NUMBER_6, GLFW_KEY_6},
        {KeyboardScancode::NUMBER_7, GLFW_KEY_7},
        {KeyboardScancode::NUMBER_8, GLFW_KEY_8},
        {KeyboardScancode::NUMBER_9, GLFW_KEY_9},
        {KeyboardScancode::SEMICOLON, GLFW_KEY_SEMICOLON},
        {KeyboardScancode::EQUALS, GLFW_KEY_EQUAL},
        {KeyboardScancode::LEFT_BRACKET, GLFW_KEY_LEFT_BRACKET},
        {KeyboardScancode::BACK_SLASH, GLFW_KEY_BACKSLASH},
        {KeyboardScancode::RIGHT_BRACKET, GLFW_KEY_RIGHT_BRACKET},
        {KeyboardScancode::GRAVE, GLFW_KEY_GRAVE_ACCENT},
        {KeyboardScancode::ESCAPE, GLFW_KEY_ESCAPE},
        {KeyboardScancode::ENTER, GLFW_KEY_ENTER},
        {KeyboardScancode::TAB, GLFW_KEY_TAB},
        {KeyboardScancode::BACKSPACE, GLFW_KEY_BACKSPACE},
        {KeyboardScancode::INSERT, GLFW_KEY_INSERT},
        {KeyboardScancode::DEL, GLFW_KEY_DELETE},
        {KeyboardScancode::ARROW_RIGHT, GLFW_KEY_RIGHT},
        {KeyboardScancode::ARROW_LEFT, GLFW_KEY_LEFT},
        {KeyboardScancode::ARROW_DOWN, GLFW_KEY_DOWN},
        {KeyboardScancode::ARROW_UP, GLFW_KEY_UP},
        {KeyboardScancode::PAGE_UP, GLFW_KEY_PAGE_UP},
        {KeyboardScancode::PAGE_DOWN, GLFW_KEY_PAGE_DOWN},
        {KeyboardScancode::HOME, GLFW_KEY_HOME},
        {KeyboardScancode::END, GLFW_KEY_END},
        {KeyboardScancode::CAPS_LOCK, GLFW_KEY_CAPS_LOCK},
        {KeyboardScancode::SCROLL_LOCK, GLFW_KEY_SCROLL_LOCK},
        {KeyboardScancode::NP_NUM_LOCK, GLFW_KEY_NUM_LOCK},
        {KeyboardScancode::PRINT_SCREEN, GLFW_KEY_PRINT_SCREEN},
        {KeyboardScancode::PAUSE, GLFW_KEY_PAUSE},
        {KeyboardScancode::F1, GLFW_KEY_F1},
        {KeyboardScancode::F2, GLFW_KEY_F2},
        {KeyboardScancode::F3, GLFW_KEY_F3},
        {KeyboardScancode::F4, GLFW_KEY_F4},
        {KeyboardScancode::F5, GLFW_KEY_F5},
        {KeyboardScancode::F6, GLFW_KEY_F6},
        {KeyboardScancode::F7, GLFW_KEY_F7},
        {KeyboardScancode::F8, GLFW_KEY_F8},
        {KeyboardScancode::F9, GLFW_KEY_F9},
        {KeyboardScancode::F10, GLFW_KEY_F10},
        {KeyboardScancode::F11, GLFW_KEY_F11},
        {KeyboardScancode::F12, GLFW_KEY_F12},
        {KeyboardScancode::NP_0, GLFW_KEY_KP_0},
        {KeyboardScancode::NP_1, GLFW_KEY_KP_1},
        {KeyboardScancode::NP_2, GLFW_KEY_KP_2},
        {KeyboardScancode::NP_3, GLFW_KEY_KP_3},
        {KeyboardScancode::NP_4, GLFW_KEY_KP_4},
        {KeyboardScancode::NP_5, GLFW_KEY_KP_5},
        {KeyboardScancode::NP_6, GLFW_KEY_KP_6},
        {KeyboardScancode::NP_7, GLFW_KEY_KP_7},
        {KeyboardScancode::NP_8, GLFW_KEY_KP_8},
        {KeyboardScancode::NP_9, GLFW_KEY_KP_9},
        {KeyboardScancode::NP_DOT, GLFW_KEY_KP_DECIMAL},
        {KeyboardScancode::NP_DIVIDE, GLFW_KEY_KP_DIVIDE},
        {KeyboardScancode::NP_TIMES, GLFW_KEY_KP_MULTIPLY},
        {KeyboardScancode::NP_MINUS, GLFW_KEY_KP_SUBTRACT},
        {KeyboardScancode::NP_PLUS, GLFW_KEY_KP_ADD},
        {KeyboardScancode::NP_ENTER, GLFW_KEY_KP_ENTER},
        {KeyboardScancode::NP_EQUALS, GLFW_KEY_KP_EQUAL},
        {KeyboardScancode::LEFT_SHIFT, GLFW_KEY_LEFT_SHIFT},
        {KeyboardScancode::LEFT_CONTROL, GLFW_KEY_LEFT_CONTROL},
        {KeyboardScancode::LEFT_ALT, GLFW_KEY_LEFT_ALT},
        {KeyboardScancode::SUPER, GLFW_KEY_LEFT_SUPER},
        {KeyboardScancode::RIGHT_SHIFT, GLFW_KEY_RIGHT_SHIFT},
        {KeyboardScancode::RIGHT_CONTROL, GLFW_KEY_RIGHT_CONTROL},
        {KeyboardScancode::RIGHT_ALT, GLFW_KEY_RIGHT_ALT},
        {KeyboardScancode::MENU, GLFW_KEY_MENU},
    });

    static KeyboardModifiers _translate_glfw_keymod(uint16_t glfw_keymod) {
        KeyboardModifiers mod = KeyboardModifiers::NONE;

        if (glfw_keymod & GLFW_MOD_SHIFT) {
            mod |= KeyboardModifiers::SHIFT;
        }
        if (glfw_keymod & GLFW_MOD_CONTROL) {
            mod |= KeyboardModifiers::CONTROL;
        }
        if (glfw_keymod & GLFW_MOD_SUPER) {
            mod |= KeyboardModifiers::SUPER;
        }
        if (glfw_keymod & GLFW_MOD_ALT) {
            mod |= KeyboardModifiers::ALT;
        }

        return mod;
    }

    static KeyboardScancode _translate_glfw_keycode(uint32_t glfw_keycode) {
        if (glfw_keycode >= GLFW_KEY_A && glfw_keycode <= GLFW_KEY_Z) {
            // GLFW shifts letter scancodes up by 61 to match ASCII
            return static_cast<KeyboardScancode>(glfw_keycode - 61);
        } else {
            auto res = g_keycode_glfw_to_argus.find(glfw_keycode);
            if (res == g_keycode_glfw_to_argus.end()) {
                _ARGUS_DEBUG("Saw unknown GLFW scancode %d\n", glfw_keycode);
                return KeyboardScancode::UNKNOWN;
            }
            return res->second;
        }
    }

    static uint32_t _translate_argus_keycode(KeyboardScancode argus_keycode) {
        if (argus_keycode >= KeyboardScancode::A && argus_keycode <= KeyboardScancode::Z) {
            // GLFW shifts letter scancodes up by 61 to match ASCII
            return static_cast<uint32_t>(argus_keycode) + 61;
        } else {
            auto res = g_keycode_argus_to_glfw.find(argus_keycode);
            if (res == g_keycode_argus_to_glfw.end()) {
                _ARGUS_WARN("Saw unknown Argus scancode %d\n", static_cast<int>(argus_keycode));
                return static_cast<uint32_t>(GLFW_KEY_UNKNOWN);
            }
            return res->second;
        }
    }

    static void _on_key_event(GLFWwindow *window, int glfw_keycode, int glfw_scancode, int glfw_action, int glfw_mods) {
        UNUSED(window);
        UNUSED(glfw_scancode);
        //TODO: determine if a key press is actually supported by Argus's API

        KeyboardEventType key_event_type;
        if (glfw_action == GLFW_PRESS) {
            key_event_type = KeyboardEventType::KEY_DOWN;
        } else if (glfw_action == GLFW_RELEASE) {
            key_event_type = KeyboardEventType::KEY_UP;
        } else {
            return;
        }

        KeyboardScancode scancode = _translate_glfw_keycode(glfw_keycode);
        KeyboardModifiers mod = _translate_glfw_keymod(static_cast<uint16_t>(glfw_mods));

        dispatch_event<KeyboardEvent>(key_event_type, scancode, mod);
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
        auto glfw_handle = static_cast<GLFWwindow*>(get_window_handle(window));
        glfwSetKeyCallback(glfw_handle, _on_key_event);
    }

    // clang-format off
    KeyboardEvent::KeyboardEvent(const KeyboardEventType subtype, const KeyboardScancode scancode, 
            const KeyboardModifiers modifiers):
            ArgusEvent(ArgusEventType::Keyboard),
            subtype(subtype),
            scancode(scancode),
            modifiers(modifiers) {
    }
    // clang-format on

    std::string KeyboardEvent::get_key_name(void) const {
        return argus::get_key_name(scancode);
    }

    std::string get_key_name(const KeyboardScancode scancode) {
        return glfwGetKeyName(0, _translate_argus_keycode(scancode));
    }

    bool is_key_down(const Window &window, const KeyboardScancode scancode) {
        int glfw_scancode = _translate_argus_keycode(scancode);
        if (glfw_scancode == GLFW_KEY_UNKNOWN) {
            return false;
        }
        return glfwGetKey(static_cast<GLFWwindow *>(get_window_handle(window)), glfw_scancode);
    }
}
