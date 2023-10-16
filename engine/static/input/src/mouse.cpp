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

#include <map>

namespace argus::input {
    static std::map<const argus::Window *, MouseState> g_mouse_states;

    argus::Vector2d mouse_pos(const argus::Window &window) {
        UNUSED(window);
        int x;
        int y;
        SDL_GetMouseState(&x, &y);

        return Vector2d(x, y);
    }

    argus::Vector2d mouse_delta(const argus::Window &window) {
        return g_mouse_states[&window].mouse_delta;
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
        _handle_mouse_events();
    }
}
