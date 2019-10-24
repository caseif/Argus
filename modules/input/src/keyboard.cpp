/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module input
#include "argus/keyboard.hpp"

// module core
#include <internal/sdl_event.hpp>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>

namespace argus {

    static bool _sdl_keyboard_event_filter(SDL_Event &event, void *const data) {
        return event.type == SDL_KEYDOWN || event.type == SDL_KEYUP;
    }

    static void _sdl_keyboard_event_handler(SDL_Event &event, void *const data) {
        //TODO: determine if a key press is actually supported by Argus's API
        KeyboardEventType key_event_type = event.type == SDL_KEYDOWN
                ? KeyboardEventType::KEY_DOWN
                : KeyboardEventType::KEY_UP;

        KeyboardScancode scancode = static_cast<KeyboardScancode>(event.key.keysym.scancode);
    }

    void init_keyboard(void) {
        register_sdl_event_handler(_sdl_keyboard_event_filter, nullptr, nullptr);
    }

    bool KeyboardEvent::is_character(void) {
        return !is_command() && !is_modifier();
    }

    bool KeyboardEvent::is_command(void) {
        //TODO
    }

    bool KeyboardEvent::is_modifier(void) {
        return is_modifier_key(scancode);
    }

    wchar_t KeyboardEvent::get_character(void) {
        //TODO
    }

    KeyboardCommand KeyboardEvent::get_command(void) {
        //TODO
    }

    KeyboardModifiers KeyboardEvent::get_modifier(void) {
        //TODO
    }

    bool is_character_key(KeyboardScancode scancode) {
        //TODO
    }

    bool is_command_key(KeyboardScancode scancode) {
        //TODO
    }

    bool is_modifier_key(KeyboardScancode scancode) {
        SDL_Keycode keycode = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode));
        return keycode == SDLK_LCTRL
                || keycode == SDLK_RCTRL
                || keycode == SDLK_LSHIFT
                || keycode == SDLK_RSHIFT
                || keycode == SDLK_LALT
                || keycode == SDLK_RALT
                || keycode == SDLK_APPLICATION
                || keycode == SDLK_NUMLOCKCLEAR
                || keycode == SDLK_CAPSLOCK
                || keycode == SDLK_SCROLLLOCK;
    }

    wchar_t get_key_character(KeyboardScancode scancode) {
        //TODO
    }

    KeyboardCommand get_key_command(KeyboardScancode scancode) {
        //TODO
    }

    KeyboardModifiers get_key_modifier(KeyboardScancode scancode) {
        //TODO
    }

}
