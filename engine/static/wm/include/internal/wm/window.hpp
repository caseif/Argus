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

#pragma once

#include "argus/core/event.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"

namespace argus {
    extern std::map<std::string, Window *> g_window_id_map;
    extern std::map<SDL_Window *, Window *> g_window_handle_map;
    extern size_t g_window_count;

    void set_window_construct_callback(WindowCallback callback);

    void window_window_event_callback(const WindowEvent &event, void *user_data);

    void peek_sdl_window_events(void);

    void reap_windows(void);

    void reset_window_displays(void);
}
