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

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"

#include "argus/wm/window.hpp"

#include "argus/input/input_manager.hpp"
#include "argus/input/keyboard.hpp"
#include "internal/input/event_helpers.hpp"
#include "internal/input/keyboard.hpp"
#include "internal/input/pimpl/controller.hpp"
#include "internal/input/pimpl/input_manager.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include "SDL_events.h"
#pragma GCC diagnostic pop
#include "SDL_keyboard.h"
#include "SDL_scancode.h"

#include <string>
#include <type_traits>
#include <unordered_map>

namespace argus::input {
    static const std::unordered_map<int, KeyboardScancode> g_scancodes_sdl_to_argus({
        { SDL_SCANCODE_SPACE,           KeyboardScancode::Space },
        { SDL_SCANCODE_APOSTROPHE,      KeyboardScancode::Apostrophe },
        { SDL_SCANCODE_COMMA,           KeyboardScancode::Comma },
        { SDL_SCANCODE_MINUS,           KeyboardScancode::Minus },
        { SDL_SCANCODE_PERIOD,          KeyboardScancode::Period },
        { SDL_SCANCODE_SLASH,           KeyboardScancode::ForwardSlash },
        { SDL_SCANCODE_0,               KeyboardScancode::Number0 },
        { SDL_SCANCODE_1,               KeyboardScancode::Number1 },
        { SDL_SCANCODE_2,               KeyboardScancode::Number2 },
        { SDL_SCANCODE_3,               KeyboardScancode::Number3 },
        { SDL_SCANCODE_4,               KeyboardScancode::Number4 },
        { SDL_SCANCODE_5,               KeyboardScancode::Number5 },
        { SDL_SCANCODE_6,               KeyboardScancode::Number6 },
        { SDL_SCANCODE_7,               KeyboardScancode::Number7 },
        { SDL_SCANCODE_8,               KeyboardScancode::Number8 },
        { SDL_SCANCODE_9,               KeyboardScancode::Number9 },
        { SDL_SCANCODE_SEMICOLON,       KeyboardScancode::Semicolon },
        { SDL_SCANCODE_EQUALS,          KeyboardScancode::Equals },
        { SDL_SCANCODE_LEFTBRACKET,     KeyboardScancode::LeftBracket },
        { SDL_SCANCODE_BACKSLASH,       KeyboardScancode::BackSlash },
        { SDL_SCANCODE_RIGHTBRACKET,    KeyboardScancode::RightBracket },
        { SDL_SCANCODE_GRAVE,           KeyboardScancode::Grave },
        { SDL_SCANCODE_ESCAPE,          KeyboardScancode::Escape },
        { SDL_SCANCODE_RETURN,          KeyboardScancode::Enter },
        { SDL_SCANCODE_TAB,             KeyboardScancode::Tab },
        { SDL_SCANCODE_BACKSPACE,       KeyboardScancode::Backspace },
        { SDL_SCANCODE_INSERT,          KeyboardScancode::Insert },
        { SDL_SCANCODE_DELETE,          KeyboardScancode::Delete },
        { SDL_SCANCODE_RIGHT,           KeyboardScancode::ArrowRight },
        { SDL_SCANCODE_LEFT,            KeyboardScancode::ArrowLeft },
        { SDL_SCANCODE_DOWN,            KeyboardScancode::ArrowDown },
        { SDL_SCANCODE_UP,              KeyboardScancode::ArrowUp },
        { SDL_SCANCODE_PAGEUP,          KeyboardScancode::PageUp },
        { SDL_SCANCODE_PAGEDOWN,        KeyboardScancode::PageDown },
        { SDL_SCANCODE_HOME,            KeyboardScancode::Home },
        { SDL_SCANCODE_END,             KeyboardScancode::End },
        { SDL_SCANCODE_CAPSLOCK,        KeyboardScancode::CapsLock },
        { SDL_SCANCODE_SCROLLLOCK,      KeyboardScancode::ScrollLock },
        { SDL_SCANCODE_NUMLOCKCLEAR,    KeyboardScancode::NumpadNumLock },
        { SDL_SCANCODE_PRINTSCREEN,     KeyboardScancode::PrintScreen },
        { SDL_SCANCODE_PAUSE,           KeyboardScancode::Pause },
        { SDL_SCANCODE_F1,              KeyboardScancode::F1 },
        { SDL_SCANCODE_F2,              KeyboardScancode::F2 },
        { SDL_SCANCODE_F3,              KeyboardScancode::F3 },
        { SDL_SCANCODE_F4,              KeyboardScancode::F4 },
        { SDL_SCANCODE_F5,              KeyboardScancode::F5 },
        { SDL_SCANCODE_F6,              KeyboardScancode::F6 },
        { SDL_SCANCODE_F7,              KeyboardScancode::F7 },
        { SDL_SCANCODE_F8,              KeyboardScancode::F8 },
        { SDL_SCANCODE_F9,              KeyboardScancode::F9 },
        { SDL_SCANCODE_F10,             KeyboardScancode::F10 },
        { SDL_SCANCODE_F11,             KeyboardScancode::F11 },
        { SDL_SCANCODE_F12,             KeyboardScancode::F12 },
        { SDL_SCANCODE_KP_0,            KeyboardScancode::Numpad0 },
        { SDL_SCANCODE_KP_1,            KeyboardScancode::Numpad1 },
        { SDL_SCANCODE_KP_2,            KeyboardScancode::Numpad2 },
        { SDL_SCANCODE_KP_3,            KeyboardScancode::Numpad3 },
        { SDL_SCANCODE_KP_4,            KeyboardScancode::Numpad4 },
        { SDL_SCANCODE_KP_5,            KeyboardScancode::Numpad5 },
        { SDL_SCANCODE_KP_6,            KeyboardScancode::Numpad6 },
        { SDL_SCANCODE_KP_7,            KeyboardScancode::Numpad7 },
        { SDL_SCANCODE_KP_8,            KeyboardScancode::Numpad8 },
        { SDL_SCANCODE_KP_9,            KeyboardScancode::Numpad9 },
        { SDL_SCANCODE_KP_DECIMAL,      KeyboardScancode::NumpadDot },
        { SDL_SCANCODE_KP_DIVIDE,       KeyboardScancode::NumpadDivide },
        { SDL_SCANCODE_KP_MULTIPLY,     KeyboardScancode::NumpadTimes },
        { SDL_SCANCODE_KP_MINUS,        KeyboardScancode::NumpadMinus },
        { SDL_SCANCODE_KP_PLUS,         KeyboardScancode::NumpadPlus },
        { SDL_SCANCODE_KP_ENTER,        KeyboardScancode::NumpadEnter },
        { SDL_SCANCODE_KP_EQUALS,       KeyboardScancode::NumpadEquals },
        { SDL_SCANCODE_LSHIFT,          KeyboardScancode::LeftShift },
        { SDL_SCANCODE_LCTRL,           KeyboardScancode::LeftControl },
        { SDL_SCANCODE_LALT,            KeyboardScancode::LeftAlt },
        { SDL_SCANCODE_LGUI,            KeyboardScancode::Super },
        { SDL_SCANCODE_RSHIFT,          KeyboardScancode::RightShift },
        { SDL_SCANCODE_RCTRL,           KeyboardScancode::RightControl },
        { SDL_SCANCODE_RALT,            KeyboardScancode::RightAlt },
        { SDL_SCANCODE_RGUI,            KeyboardScancode::Super },
        { SDL_SCANCODE_MENU,            KeyboardScancode::Menu },
    });
    static const std::unordered_map<KeyboardScancode, int> g_scancodes_argus_to_sdl({
        { KeyboardScancode::Space,          SDL_SCANCODE_SPACE },
        { KeyboardScancode::Apostrophe,     SDL_SCANCODE_APOSTROPHE },
        { KeyboardScancode::Comma,          SDL_SCANCODE_COMMA },
        { KeyboardScancode::Minus,          SDL_SCANCODE_MINUS },
        { KeyboardScancode::Period,         SDL_SCANCODE_PERIOD },
        { KeyboardScancode::ForwardSlash,   SDL_SCANCODE_SLASH },
        { KeyboardScancode::Number0,        SDL_SCANCODE_0 },
        { KeyboardScancode::Number1,        SDL_SCANCODE_1 },
        { KeyboardScancode::Number2,        SDL_SCANCODE_2 },
        { KeyboardScancode::Number3,        SDL_SCANCODE_3 },
        { KeyboardScancode::Number4,        SDL_SCANCODE_4 },
        { KeyboardScancode::Number5,        SDL_SCANCODE_5 },
        { KeyboardScancode::Number6,        SDL_SCANCODE_6 },
        { KeyboardScancode::Number7,        SDL_SCANCODE_7 },
        { KeyboardScancode::Number8,        SDL_SCANCODE_8 },
        { KeyboardScancode::Number9,        SDL_SCANCODE_9 },
        { KeyboardScancode::Semicolon,      SDL_SCANCODE_SEMICOLON },
        { KeyboardScancode::Equals,         SDL_SCANCODE_EQUALS },
        { KeyboardScancode::LeftBracket,    SDL_SCANCODE_LEFTBRACKET },
        { KeyboardScancode::BackSlash,      SDL_SCANCODE_BACKSLASH },
        { KeyboardScancode::RightBracket,   SDL_SCANCODE_RIGHTBRACKET },
        { KeyboardScancode::Grave,          SDL_SCANCODE_GRAVE },
        { KeyboardScancode::Escape,         SDL_SCANCODE_ESCAPE },
        { KeyboardScancode::Enter,          SDL_SCANCODE_RETURN },
        { KeyboardScancode::Tab,            SDL_SCANCODE_TAB },
        { KeyboardScancode::Backspace,      SDL_SCANCODE_BACKSPACE },
        { KeyboardScancode::Insert,         SDL_SCANCODE_INSERT },
        { KeyboardScancode::Delete,         SDL_SCANCODE_DELETE },
        { KeyboardScancode::ArrowRight,     SDL_SCANCODE_RIGHT },
        { KeyboardScancode::ArrowLeft,      SDL_SCANCODE_LEFT },
        { KeyboardScancode::ArrowDown,      SDL_SCANCODE_DOWN },
        { KeyboardScancode::ArrowUp,        SDL_SCANCODE_UP },
        { KeyboardScancode::PageUp,         SDL_SCANCODE_PAGEUP },
        { KeyboardScancode::PageDown,       SDL_SCANCODE_PAGEDOWN },
        { KeyboardScancode::Home,           SDL_SCANCODE_HOME },
        { KeyboardScancode::End,            SDL_SCANCODE_END },
        { KeyboardScancode::CapsLock,       SDL_SCANCODE_CAPSLOCK },
        { KeyboardScancode::ScrollLock,     SDL_SCANCODE_SCROLLLOCK },
        { KeyboardScancode::NumpadNumLock,  SDL_SCANCODE_NUMLOCKCLEAR },
        { KeyboardScancode::PrintScreen,    SDL_SCANCODE_PRINTSCREEN },
        { KeyboardScancode::Pause,          SDL_SCANCODE_PAUSE },
        { KeyboardScancode::F1,             SDL_SCANCODE_F1 },
        { KeyboardScancode::F2,             SDL_SCANCODE_F2 },
        { KeyboardScancode::F3,             SDL_SCANCODE_F3 },
        { KeyboardScancode::F4,             SDL_SCANCODE_F4 },
        { KeyboardScancode::F5,             SDL_SCANCODE_F5 },
        { KeyboardScancode::F6,             SDL_SCANCODE_F6 },
        { KeyboardScancode::F7,             SDL_SCANCODE_F7 },
        { KeyboardScancode::F8,             SDL_SCANCODE_F8 },
        { KeyboardScancode::F9,             SDL_SCANCODE_F9 },
        { KeyboardScancode::F10,            SDL_SCANCODE_F10 },
        { KeyboardScancode::F11,            SDL_SCANCODE_F11 },
        { KeyboardScancode::F12,            SDL_SCANCODE_F12 },
        { KeyboardScancode::Numpad0,        SDL_SCANCODE_KP_0 },
        { KeyboardScancode::Numpad1,        SDL_SCANCODE_KP_1 },
        { KeyboardScancode::Numpad2,        SDL_SCANCODE_KP_2 },
        { KeyboardScancode::Numpad3,        SDL_SCANCODE_KP_3 },
        { KeyboardScancode::Numpad4,        SDL_SCANCODE_KP_4 },
        { KeyboardScancode::Numpad5,        SDL_SCANCODE_KP_5 },
        { KeyboardScancode::Numpad6,        SDL_SCANCODE_KP_6 },
        { KeyboardScancode::Numpad7,        SDL_SCANCODE_KP_7 },
        { KeyboardScancode::Numpad8,        SDL_SCANCODE_KP_8 },
        { KeyboardScancode::Numpad9,        SDL_SCANCODE_KP_9 },
        { KeyboardScancode::NumpadDot,      SDL_SCANCODE_KP_DECIMAL },
        { KeyboardScancode::NumpadDivide,   SDL_SCANCODE_KP_DIVIDE },
        { KeyboardScancode::NumpadTimes,    SDL_SCANCODE_KP_MULTIPLY },
        { KeyboardScancode::NumpadMinus,    SDL_SCANCODE_KP_MINUS },
        { KeyboardScancode::NumpadPlus,     SDL_SCANCODE_KP_PLUS },
        { KeyboardScancode::NumpadEnter,    SDL_SCANCODE_KP_ENTER },
        { KeyboardScancode::NumpadEquals,   SDL_SCANCODE_KP_EQUALS },
        { KeyboardScancode::LeftShift,      SDL_SCANCODE_LSHIFT },
        { KeyboardScancode::LeftControl,    SDL_SCANCODE_LCTRL },
        { KeyboardScancode::LeftAlt,        SDL_SCANCODE_LALT },
        { KeyboardScancode::Super,          SDL_SCANCODE_LGUI },
        { KeyboardScancode::RightShift,     SDL_SCANCODE_RSHIFT },
        { KeyboardScancode::RightControl,   SDL_SCANCODE_RCTRL },
        { KeyboardScancode::RightAlt,       SDL_SCANCODE_RALT },
        { KeyboardScancode::Super,          SDL_SCANCODE_RGUI },
        { KeyboardScancode::Menu,           SDL_SCANCODE_MENU },
    });

    static KeyboardScancode _translate_sdl_scancode(int sdl_scancode) {
        if (sdl_scancode < 0) {
            Logger::default_logger().warn("Received negative keyboard scancode %d", sdl_scancode);
            return KeyboardScancode::Unknown;
        }

        if (sdl_scancode >= SDL_SCANCODE_A && sdl_scancode <= SDL_SCANCODE_Z) {
            // USB scancodes for letter keys are offset from ASCII by 61
            return static_cast<KeyboardScancode>(sdl_scancode);
        } else {
            auto res = g_scancodes_sdl_to_argus.find(sdl_scancode);
            if (res == g_scancodes_sdl_to_argus.end()) {
                Logger::default_logger().debug("Received unknown keyboard scancode %d", sdl_scancode);
                return KeyboardScancode::Unknown;
            }
            return res->second;
        }
    }

    static SDL_Scancode _translate_argus_scancode(KeyboardScancode argus_scancode) {
        if (argus_scancode >= KeyboardScancode::A && argus_scancode <= KeyboardScancode::Z) {
            // SDL2 shifts letter scancodes up by 61 to match ASCII
            return SDL_Scancode(int(argus_scancode));
        } else {
            auto res = g_scancodes_argus_to_sdl.find(argus_scancode);
            if (res == g_scancodes_argus_to_sdl.end()) {
                Logger::default_logger().warn("Saw unknown Argus scancode %d", int(argus_scancode));
                return SDL_SCANCODE_UNKNOWN;
            }
            return SDL_Scancode(res->second);
        }
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
        UNUSED(window);
    }

    std::string get_key_name(KeyboardScancode scancode) {
        return SDL_GetKeyName(SDL_GetKeyFromScancode(_translate_argus_scancode(scancode)));
    }

    bool is_key_pressed(KeyboardScancode scancode) {
        std::lock_guard<std::mutex> lock(InputManager::instance().pimpl->keyboard_state_mutex);

        auto state = InputManager::instance().pimpl->keyboard_state;
        if (state == nullptr) {
            return false;
        }

        int sdl_scancode = _translate_argus_scancode(scancode);
        if (sdl_scancode == SDL_SCANCODE_UNKNOWN) {
            return false;
        }

        if (sdl_scancode >= InputManager::instance().pimpl->keyboard_key_count) {
            return false;
        }

        return state[sdl_scancode] != 0;
    }

    static void _poll_keyboard_state(void) {
        std::lock_guard<std::mutex> lock(InputManager::instance().pimpl->keyboard_state_mutex);
        InputManager::instance().pimpl->keyboard_state
                = SDL_GetKeyboardState(&InputManager::instance().pimpl->keyboard_key_count);
    }

    static void _dispatch_events(const Window &window, KeyboardScancode key, bool release) {
        //TODO: ignore while in a TextInputContext once we properly implement that

        for (auto &pair : InputManager::instance().pimpl->controllers) {
            auto controller_index = pair.first;
            auto &controller = *pair.second;

            auto it = controller.pimpl->key_to_action_bindings.find(key);
            if (it == controller.pimpl->key_to_action_bindings.end()) {
                continue;
            }

            for (auto &action : it->second) {
                dispatch_button_event(&window, controller_index, action, release);
            }
        }
    }

    static void _handle_keyboard_events(void) {
        constexpr size_t event_buf_size = 8;
        SDL_Event events[event_buf_size];

        int to_process;
        while ((to_process = SDL_PeepEvents(events, event_buf_size, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP)) > 0) {
            for (int i = 0; i < to_process; i++) {
                auto &event = events[i];
                if (event.key.repeat) {
                    continue;
                }
                auto *window = get_window_from_handle(SDL_GetWindowFromID(event.key.windowID));
                if (window == nullptr) {
                    return;
                }

                auto key = _translate_sdl_scancode(event.key.keysym.scancode);
                _dispatch_events(*window, key, event.type == SDL_KEYUP);
            }
        }
    }

    void update_keyboard(void) {
        _poll_keyboard_state();
        _handle_keyboard_events();
    }
}
