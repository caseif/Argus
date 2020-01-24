/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
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

// module renderer
#include "argus/renderer/window.hpp"

// module input
#include "argus/keyboard.hpp"

#include <GLFW/glfw3.h>

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

    static void _on_key_event(GLFWwindow *window, int glfw_key, int glfw_scancode, int glfw_action, int glfw_mods) {
        //TODO: determine if a key press is actually supported by Argus's API

        KeyboardEventType key_event_type;
        if (glfw_action == GLFW_PRESS) {
            key_event_type = KeyboardEventType::KEY_DOWN;
        } else if (glfw_action == GLFW_RELEASE) {
            key_event_type = KeyboardEventType::KEY_UP;
        } else {
            return;
        }

        KeyboardScancode scancode = static_cast<KeyboardScancode>(glfw_scancode);

        KeyboardModifiers mod = _translate_glfw_keymod(glfw_mods);

        dispatch_event(KeyboardEvent(key_event_type, scancode, mod));
    }

    static void _update_callback(TimeDelta delta) {
        g_last_keyboard_state = SDL_GetKeyboardState(&g_keyboard_key_count);
    }

    constexpr inline KeyboardModifiers operator |(const KeyboardModifiers lhs, const KeyboardModifiers rhs) {
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

    constexpr inline KeyboardModifiers operator &(const KeyboardModifiers lhs, const KeyboardModifiers rhs) {
        return static_cast<KeyboardModifiers>(
                static_cast<std::underlying_type<KeyboardModifiers>::type>(lhs)
                & static_cast<std::underlying_type<KeyboardModifiers>::type>(rhs)
        );
    }

    void init_keyboard(GLFWwindow *handle) {
        glfwSetKeyCallback(handle, _on_key_event);
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
            case GLFW_MOD_CONTROL:
                return KeyboardModifiers::CONTROL;
            case GLFW_MOD_SHIFT:
                return KeyboardModifiers::SHIFT;
            case GLFW_MOD_ALT:
                return KeyboardModifiers::ALT;
            case GLFW_MOD_SUPER:
                return KeyboardModifiers::SUPER;
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
