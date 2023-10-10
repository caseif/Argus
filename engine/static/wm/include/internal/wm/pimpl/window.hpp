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

#pragma once

#include "argus/lowlevel/atomic.hpp"
#include "argus/lowlevel/math.hpp"

#include "argus/core/callback.hpp"

#include "argus/wm/window.hpp"

#include <atomic>
#include <string>
#include <vector>

#include "SDL_video.h"

namespace argus {
    struct pimpl_Window {
        /**
         * \brief A handle to the lower-level window represented by this
         *        object.
         */
        SDL_Window *handle;

        /**
         * \brief The unique identifier of the window.
         */
        std::string id;

        /**
         * \brief The Canvas associated with this Window.
         *
         * This is only set if the Canvas constructor has been set by the module
         * responsible for implementing canvases.
         */
        Canvas *canvas;

        /**
         * \brief The ID of the engine callback registered for this Window.
         */
        Index callback_id;

        /**
         * \brief The Window parent to this one, if applicable.
         */
        Window *parent;
        /**
         * \brief This Window's child \link Window Windows \endlink, if any.
         */
        std::vector<Window *> children;

        struct {
            AtomicDirtiable<std::string> title;
            AtomicDirtiable<bool> fullscreen;
            AtomicDirtiable<const Display *> display;
            AtomicDirtiable<bool> custom_display_mode;
            AtomicDirtiable<DisplayMode> display_mode;
            AtomicDirtiable<Vector2u> windowed_resolution;
            AtomicDirtiable<Vector2i> position;
            AtomicDirtiable<bool> vsync;
            AtomicDirtiable<bool> mouse_capture;
            AtomicDirtiable<bool> mouse_visible;
            AtomicDirtiable<bool> mouse_raw_input;
        } properties;

        Vector2f content_scale;

        /**
         * \brief The callback to be executed upon the Window being closed.
         */
        WindowCallback close_callback;

        /**
         * \brief The state of this Window as a bitfield.
         *
         * \warning This field's semantic meaning is implementation-defined.
         */
        std::atomic<unsigned int> state;

        AtomicDirtiable<Vector2u> cur_resolution;

        uint16_t cur_refresh_rate;

        std::atomic_int refcount;

        pimpl_Window(const std::string &id, Window *parent) :
                id(id),
                parent(parent) {
        }

        pimpl_Window(const pimpl_Window &) = delete;

        pimpl_Window(pimpl_Window &&) = delete;
    };
}
