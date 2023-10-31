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

#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/math.hpp"

#include "argus/wm/window.hpp"

#include "argus/input/input_manager.hpp"
#include "argus/input/mouse.hpp"
#include "internal/input/mouse.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include "SDL_events.h"
#pragma GCC diagnostic pop
#include "SDL_mouse.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace argus::input {
    static const std::unordered_map<MouseButton, int> g_mouse_button_mappings({
            { MouseButton::Primary, SDL_BUTTON_LEFT },
            { MouseButton::Middle, SDL_BUTTON_MIDDLE },
            { MouseButton::Secondary, SDL_BUTTON_RIGHT },
            { MouseButton::Back, SDL_BUTTON_X1 },
            { MouseButton::Forward, SDL_BUTTON_X2 },
    });

    static MouseState g_mouse_state;
    static std::mutex g_mouse_state_mutex;

    argus::Vector2d mouse_pos(void) {
        std::lock_guard<std::mutex> lock(g_mouse_state_mutex);
        return g_mouse_state.last_mouse_pos;
    }

    argus::Vector2d mouse_delta(void) {
        std::lock_guard<std::mutex> lock(g_mouse_state_mutex);
        return g_mouse_state.mouse_delta;
    }

    double get_mouse_axis(MouseAxis axis) {
        std::lock_guard<std::mutex> lock(g_mouse_state_mutex);
        switch (axis) {
            case MouseAxis::Horizontal:
                return g_mouse_state.last_mouse_pos.x;
            case MouseAxis::Vertical:
                return g_mouse_state.last_mouse_pos.y;
            default:
                throw std::invalid_argument("Unknown mouse axis ordinal " + std::to_string(int(axis)));
        }
    }

    double get_mouse_axis_delta(MouseAxis axis) {
        std::lock_guard<std::mutex> lock(g_mouse_state_mutex);
        double val;
        switch (axis) {
            case MouseAxis::Horizontal:
                val = g_mouse_state.mouse_delta.x;
                break;
            case MouseAxis::Vertical:
                val = g_mouse_state.mouse_delta.y;
                break;
            default:
                throw std::invalid_argument("Unknown mouse axis ordinal " + std::to_string(int(axis)));
        }
        g_mouse_state.is_delta_stale = true;
        return val;
    }

    bool is_mouse_button_pressed(MouseButton button) {
        std::lock_guard<std::mutex> lock(g_mouse_state_mutex);

        if (int(button) < 0) {
            throw std::invalid_argument("Invalid mouse button ordinal " + std::to_string(int(button)));
        }
        auto sdl_button = g_mouse_button_mappings.find(button);
        if (sdl_button == g_mouse_button_mappings.cend()) {
            throw std::invalid_argument("Invalid mouse button ordinal " + std::to_string(int(button)));
        }
        return (g_mouse_state.button_state & SDL_BUTTON(sdl_button->second)) != 0;
    }

    static void _poll_mouse(void) {
        std::lock_guard<std::mutex> lock(g_mouse_state_mutex);

        int x;
        int y;
        g_mouse_state.button_state = SDL_GetMouseState(&x, &y);

        if (g_mouse_state.got_first_mouse_pos) {
            g_mouse_state.mouse_delta.x = double(x) - g_mouse_state.last_mouse_pos.x;
            g_mouse_state.mouse_delta.y = double(y) - g_mouse_state.last_mouse_pos.y;
        } else {
            g_mouse_state.got_first_mouse_pos = true;
        }

        g_mouse_state.last_mouse_pos.x = double(x);
        g_mouse_state.last_mouse_pos.y = double(y);

        g_mouse_state.is_delta_stale = false;
    }

    static void _handle_mouse_events(void) {
        constexpr size_t event_buf_size = 8;
        SDL_Event events[event_buf_size];

        int to_process;
        while ((to_process = SDL_PeepEvents(events, event_buf_size,
                SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEBUTTONUP)) > 0) {
            for (int i = 0; i < to_process; i++) {
                auto &event = events[i];

                auto *window = get_window_from_handle(SDL_GetWindowFromID(event.key.windowID));
                if (window == nullptr) {
                    return;
                }

                if (event.type == SDL_MOUSEMOTION) {
                    InputManager::instance().handle_mouse_axis_change(*window, MouseAxis::Horizontal,
                            event.motion.x, event.motion.xrel);
                    InputManager::instance().handle_mouse_axis_change(*window, MouseAxis::Vertical,
                            event.motion.y, event.motion.yrel);
                } else {
                    MouseButton button;
                    switch (event.button.button) {
                        case SDL_BUTTON_LEFT:
                            button = MouseButton::Primary;
                            break;
                        case SDL_BUTTON_RIGHT:
                            button = MouseButton::Secondary;
                            break;
                        case SDL_BUTTON_MIDDLE:
                            button = MouseButton::Middle;
                            break;
                        case SDL_BUTTON_X1:
                            button = MouseButton::Back;
                            break;
                        case SDL_BUTTON_X2:
                            button = MouseButton::Forward;
                            break;
                        default:
                            Logger::default_logger().debug("Ignoring unrecognized mouse button with ordinal %d",
                                    event.button.which);
                            return;
                    }
                    InputManager::instance().handle_mouse_button_press(*window, button,
                            event.type == SDL_MOUSEBUTTONUP);
                }
            }
        }
    }

    void init_mouse(const argus::Window &window) {
        UNUSED(window);
        // no-op
    }

    void update_mouse(void) {
        _poll_mouse();
        _handle_mouse_events();
    }

    void flush_mouse_delta(void) {
        std::lock_guard<std::mutex> lock(g_mouse_state_mutex);
        if (g_mouse_state.is_delta_stale) {
            g_mouse_state.mouse_delta = {};
        }
    }
}
