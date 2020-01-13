/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core/core_util.hpp"
#include "internal/core/sdl_event.hpp"

// module input
#include "argus/keyboard.hpp"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <cstdint>

namespace argus {

    static std::vector<TextInputContext*> g_input_contexts;
    static TextInputContext *g_active_input_context = nullptr;

    static const uint8_t *g_last_keyboard_state = nullptr;
    static int g_keyboard_key_count = 0;

    static KeyboardModifiers _translate_sdl_keymod(uint16_t sdl_keymod) {
        KeyboardModifiers mod = KeyboardModifiers::NONE;
        
        if (sdl_keymod & KMOD_NUM) {
            mod |= KeyboardModifiers::NUM_LOCK;
        }
        if (sdl_keymod & KMOD_CAPS) {
            mod |= KeyboardModifiers::CAPS_LOCK;
        }
        if (sdl_keymod & KMOD_LCTRL) {
            mod |= KeyboardModifiers::LEFT_CONTROL;
        }
        if (sdl_keymod & KMOD_RCTRL) {
            mod |= KeyboardModifiers::RIGHT_CONTROL;
        }
        if (sdl_keymod & KMOD_LSHIFT) {
            mod |= KeyboardModifiers::LEFT_SHIFT;
        }
        if (sdl_keymod & KMOD_RSHIFT) {
            mod |= KeyboardModifiers::RIGHT_SHIFT;
        }
        if (sdl_keymod & KMOD_LALT) {
            mod |= KeyboardModifiers::LEFT_ALT;
        }
        if (sdl_keymod & KMOD_RALT) {
            mod |= KeyboardModifiers::RIGHT_ALT;
        }

        return mod;
    }

    static bool _sdl_keyboard_event_filter(SDL_Event &sdl_event, void *const data) {
        return sdl_event.type == SDL_KEYDOWN || sdl_event.type == SDL_KEYUP;
    }

    static void _sdl_keyboard_event_handler(SDL_Event &sdl_event, void *const data) {
        //TODO: determine if a key press is actually supported by Argus's API

        KeyboardEventType key_event_type = sdl_event.type == SDL_KEYDOWN
                ? KeyboardEventType::KEY_DOWN
                : KeyboardEventType::KEY_UP;

        KeyboardScancode scancode = static_cast<KeyboardScancode>(sdl_event.key.keysym.scancode);

        KeyboardModifiers mod = _translate_sdl_keymod(sdl_event.key.keysym.mod);

        dispatch_event(KeyboardEvent(key_event_type, scancode, mod));
    }

    static void _update_callback(TimeDelta delta) {
        g_last_keyboard_state = SDL_GetKeyboardState(&g_keyboard_key_count);
    }

    KeyboardModifiers operator |(const KeyboardModifiers lhs, const KeyboardModifiers rhs) {
        return static_cast<KeyboardModifiers>(
                static_cast<std::underlying_type<KeyboardModifiers>::type>(lhs)
                | static_cast<std::underlying_type<KeyboardModifiers>::type>(rhs)
        );
    }

    constexpr inline KeyboardModifiers operator |=(const KeyboardModifiers lhs, const KeyboardModifiers rhs) {
        return static_cast<KeyboardModifiers>(
                static_cast<std::underlying_type<KeyboardModifiers>::type>(lhs)
                | static_cast<std::underlying_type<KeyboardModifiers>::type>(rhs)
        );
    }

    inline bool operator &(const KeyboardModifiers lhs, const KeyboardModifiers rhs) {
        return (static_cast<std::underlying_type<KeyboardModifiers>::type>(lhs)
                & static_cast<std::underlying_type<KeyboardModifiers>::type>(rhs));
    }

    void init_keyboard(void) {
        register_sdl_event_handler(_sdl_keyboard_event_filter, _sdl_keyboard_event_handler, nullptr);
    }

    KeyboardEvent::KeyboardEvent(const KeyboardEventType subtype, const KeyboardScancode scancode, 
            const KeyboardModifiers modifiers):
            ArgusEvent(ArgusEventType::KEYBOARD),
            subtype(subtype),
            scancode(scancode),
            modifiers(modifiers) {
    }

    const std::string KeyboardEvent::get_key_name(void) const {
        return argus::get_key_name(scancode);
    }

    const bool KeyboardEvent::is_command(void) const {
        return is_command_key(scancode);
    }

    const bool KeyboardEvent::is_modifier(void) const {
        return is_modifier_key(scancode);
    }

    const KeyboardCommand KeyboardEvent::get_command(void) const {
        return get_key_command(scancode);
    }

    const KeyboardModifiers KeyboardEvent::get_modifier(void) const {
        return get_key_modifier(scancode);
    }

    const bool is_command_key(const KeyboardScancode scancode) {
        SDL_Keycode keycode = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode));
        return keycode == SDLK_RETURN
                || keycode == SDLK_ESCAPE
                || keycode == SDLK_BACKSPACE
                || keycode == SDLK_TAB
                || keycode == SDLK_PRINTSCREEN
                || keycode == SDLK_SCROLLLOCK
                || keycode == SDLK_PAUSE
                || keycode == SDLK_INSERT
                || keycode == SDLK_HOME
                || keycode == SDLK_PAGEUP
                || keycode == SDLK_DELETE
                || keycode == SDLK_END
                || keycode == SDLK_PAGEDOWN
                || keycode == SDLK_RIGHT
                || keycode == SDLK_LEFT
                || keycode == SDLK_DOWN
                || keycode == SDLK_UP
                || keycode == SDLK_KP_ENTER
                || keycode == SDLK_MENU
                || keycode == SDLK_LGUI;
    }

    const bool is_modifier_key(const KeyboardScancode scancode) {
        SDL_Keycode keycode = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode));
        return keycode == SDLK_LCTRL
                || keycode == SDLK_RCTRL
                || keycode == SDLK_LSHIFT
                || keycode == SDLK_RSHIFT
                || keycode == SDLK_LALT
                || keycode == SDLK_RALT
                || keycode == SDLK_NUMLOCKCLEAR
                || keycode == SDLK_CAPSLOCK
                || keycode == SDLK_SCROLLLOCK;
    }

    const std::string get_key_name(const KeyboardScancode scancode) {
        SDL_Keycode keycode = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode));
        return SDL_GetKeyName(keycode);
    }

    const KeyboardCommand get_key_command(const KeyboardScancode scancode) {
        SDL_Keycode keycode = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode));
        switch (keycode) {
            case SDLK_RETURN:
                return KeyboardCommand::ENTER;
            case SDLK_ESCAPE:
                return KeyboardCommand::ESCAPE;
            case SDLK_BACKSPACE:
                return KeyboardCommand::BACKSPACE;
            case SDLK_TAB:
                return KeyboardCommand::TAB;
            case SDLK_PRINTSCREEN:
                return KeyboardCommand::PRINT_SCREEN;
            case SDLK_SCROLLLOCK:
                return KeyboardCommand::SCROLL_LOCK;
            case SDLK_PAUSE:
                return KeyboardCommand::PAGE_UP;
            case SDLK_INSERT:
                return KeyboardCommand::INSERT;
            case SDLK_HOME:
                return KeyboardCommand::HOME;
            case SDLK_PAGEUP:
                return KeyboardCommand::PAGE_UP;
            case SDLK_DELETE:
                return KeyboardCommand::DELETE;
            case SDLK_END:
                return KeyboardCommand::END;
            case SDLK_PAGEDOWN:
                return KeyboardCommand::PAGE_DOWN;
            case SDLK_RIGHT:
                return KeyboardCommand::ARROW_RIGHT;
            case SDLK_LEFT:
                return KeyboardCommand::ARROW_LEFT;
            case SDLK_DOWN:
                return KeyboardCommand::ARROW_DOWN;
            case SDLK_UP:
                return KeyboardCommand::ARROW_UP;
            case SDLK_KP_ENTER:
                return KeyboardCommand::NP_ENTER;
            case SDLK_MENU:
                return KeyboardCommand::MENU;
            case SDLK_LGUI:
                return KeyboardCommand::SUPER;
        }
    }

    const KeyboardModifiers get_key_modifier(const KeyboardScancode scancode) {
        if (!is_modifier_key(scancode)) {
            throw std::invalid_argument("get_modifier called for non-modifier key");
        }

        SDL_Keycode keycode = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode));
        switch (keycode) {
            case SDLK_LCTRL:
                return KeyboardModifiers::LEFT_CONTROL;
            case SDLK_RCTRL:
                return KeyboardModifiers::RIGHT_CONTROL;
            case SDLK_LSHIFT:
                return KeyboardModifiers::LEFT_SHIFT;
            case SDLK_RSHIFT:
                return KeyboardModifiers::RIGHT_SHIFT;
            case SDLK_LALT:
                return KeyboardModifiers::LEFT_ALT;
            case SDLK_RALT:
                return KeyboardModifiers::RIGHT_ALT;
            case SDLK_NUMLOCKCLEAR:
                return KeyboardModifiers::NUM_LOCK;
            case SDLK_CAPSLOCK:
                return KeyboardModifiers::CAPS_LOCK;
            case SDLK_SCROLLLOCK:
                return KeyboardModifiers::SCROLL_LOCK;
            default:
                _ARGUS_FATAL("Unsupported key modifier %d\n", keycode);
        }
    }

    const bool is_key_down(const KeyboardScancode scancode) {
        SDL_Scancode sdl_scancode = static_cast<SDL_Scancode>(scancode);
        if (sdl_scancode >= g_keyboard_key_count) {
            return false;
        }
        return g_last_keyboard_state[sdl_scancode];
    }

    TextInputContext::TextInputContext(void):
            valid(true),
            active(false),
            text() {
        this->activate();
    }

    TextInputContext &TextInputContext::create_context(void) {
        return *new TextInputContext();
    }

    const std::string &TextInputContext::get_current_text(void) const {
        return text;
    }

    void TextInputContext::activate(void) {
        if (g_active_input_context != nullptr) {
            g_active_input_context->deactivate();
        }

        this->active = true;
        g_active_input_context = this;
    }

    void TextInputContext::deactivate(void) {
        if (!this->active) {
            return;
        }

        this->active = false;
        g_active_input_context = nullptr;
    }

    void TextInputContext::release(void) {
        this->deactivate();
        this->valid = false;
        remove_from_vector(g_input_contexts, this);
    }

}
