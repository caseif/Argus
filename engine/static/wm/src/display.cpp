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

#include "argus/wm/display.hpp"
#include "argus/wm/window.hpp"
#include "internal/wm/display.hpp"
#include "internal/wm/window.hpp"
#include "internal/wm/pimpl/display.hpp"
#include "internal/wm/pimpl/window.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include "SDL_events.h"
#pragma GCC diagnostic pop
#include "SDL_video.h"

#include <string>
#include <vector>

namespace argus {
    static std::vector<const Display *> g_displays;

    // ideally we'd use the POPCNT instruction on x86, but this is only expected
    // to be called during init
    static uint8_t sw_popcnt(uint32_t val) {
        uint32_t val_shifted = val;
        uint8_t count = 0;
        for (uint32_t i = 0; i < sizeof(val) * 8; i++) {
            count += val_shifted & 1;
            val_shifted >>= 1;
        }
        return count;
    }

    DisplayMode wrap_display_mode(SDL_DisplayMode mode) {
        affirm_precond(mode.w > 0 && mode.h > 0, "Display mode dimensions must be greater than 0");

        int bpp;
        uint32_t mask_r;
        uint32_t mask_g;
        uint32_t mask_b;
        uint32_t mask_a;
        if (SDL_PixelFormatEnumToMasks(mode.format, &bpp, &mask_r, &mask_g, &mask_b, &mask_a) != SDL_TRUE) {
            Logger::default_logger().warn("Failed to query color channels modes for display mode (%s)",
                    SDL_GetError());
        }

        uint8_t bits_r = sw_popcnt(mask_r);
        uint8_t bits_g = sw_popcnt(mask_g);
        uint8_t bits_b = sw_popcnt(mask_b);
        uint8_t bits_a = sw_popcnt(mask_a);

        return DisplayMode {
                Vector2u(uint32_t(mode.w), uint32_t(mode.h)),
                uint16_t(mode.refresh_rate),
                Vector4u(uint32_t(bits_r), uint32_t(bits_g), uint32_t(bits_b), uint32_t(bits_a)),
                mode.format
        };
    }

    SDL_DisplayMode unwrap_display_mode(const DisplayMode &mode) {
        return SDL_DisplayMode {
            mode.extra_data,
            int(mode.resolution.x), int(mode.resolution.y),
            mode.refresh_rate,
            nullptr
        };
    }

    static Display *_add_display(int index) {
        auto &display = *new Display();

        display.pimpl->index = index;

        const char *display_name = SDL_GetDisplayName(index);
        if (display_name == nullptr) {
            Logger::default_logger().warn("Failed to query name of display %d (%s)", index, SDL_GetError());
        }

        display.pimpl->name = display_name;

        SDL_Rect bounds;
        if (SDL_GetDisplayBounds(index, &bounds) != 0) {
            Logger::default_logger().warn("Failed to query bounds of display %d (%s)", index, SDL_GetError());
        }
        display.pimpl->position.x = bounds.x;
        display.pimpl->position.y = bounds.y;

        int mode_count = SDL_GetNumDisplayModes(display.pimpl->index);
        if (mode_count < 0) {
            Logger::default_logger().warn("Failed to query display modes for display %d (%s)", index, SDL_GetError());
        }

        for (int i = 0; i < mode_count; i++) {
            SDL_DisplayMode mode;
            if (SDL_GetDisplayMode(display.pimpl->index, i, &mode) != 0) {
                Logger::default_logger().warn("Failed to query display mode %d for display %d, skipping (%s)",
                        i, display.pimpl->index, SDL_GetError());
                continue;
            }

            display.pimpl->modes.push_back(wrap_display_mode(mode));
        }

        return &display;
    }

    static void _enumerate_displays(std::vector<const Display *> &dest) {
        std::vector<Display> displays;

        int count = SDL_GetNumVideoDisplays();
        if (count < 0) {
            Logger::default_logger().fatal("Failed to enumerate displays");
        }
        dest.resize(size_t(count));
        for (int i = 0; i < count; i++) {
            dest[size_t(i)] = _add_display(i);
        }
    }

    static void _update_displays(void) {
        std::vector<const Display *> old_displays;
        std::vector<const Display *> new_displays;

        _enumerate_displays(new_displays);

        reset_window_displays();

        g_displays = new_displays;

        for (auto *display : old_displays) {
            delete display;
        }
    }

    static int _display_callback(void *udata, SDL_Event *event) {
        UNUSED(udata);

        if (event->type != SDL_DISPLAYEVENT) {
            return 0;
        }

        SDL_DisplayEvent disp_event = event->display;

        if (disp_event.type == SDL_DISPLAYEVENT_CONNECTED
                || disp_event.type == SDL_DISPLAYEVENT_DISCONNECTED) {
            _update_displays();
        }

        return 0;
    }

    void init_display(void) {
        _enumerate_displays(g_displays);

        SDL_AddEventWatch(_display_callback, nullptr);
    }

    const Display *get_display_from_index(int index) {
        return g_displays[uint32_t(index)];
    }

    const std::vector<const Display *> &Display::get_available_displays(void) {
        return g_displays;
    }

    Display::Display(void) :
            pimpl(new pimpl_Display()) {
    }

    Display::~Display(void) {
        delete pimpl;
    }

    const std::string &Display::get_name(void) const {
        return pimpl->name;
    }

    Vector2i Display::get_position(void) const {
        return pimpl->position;
    }

    const std::vector<DisplayMode> &Display::get_display_modes(void) const {
        return pimpl->modes;
    }
}
