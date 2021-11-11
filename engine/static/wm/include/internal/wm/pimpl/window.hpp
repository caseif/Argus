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

#pragma once

// module lowlevel
#include "argus/lowlevel/atomic.hpp"
#include "argus/lowlevel/math.hpp"

// module core
#include "argus/core/callback.hpp"

// module wm
#include "argus/wm/window.hpp"

#include <atomic>
#include <vector>

// forward declarations
struct GLFWwindow;

namespace argus {
    struct pimpl_Window {
        /**
         * \brief A handle to the lower-level window represented by this
         *        object.
         */
        GLFWwindow *handle;

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
        std::vector<Window*> children;

        struct {
            AtomicDirtiable<std::string> title;
            AtomicDirtiable<bool> fullscreen;
            AtomicDirtiable<Vector2u> resolution;
            AtomicDirtiable<Vector2i> position;
            AtomicDirtiable<bool> vsync;
        } properties;

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

        /**
         * \brief Whether the render resolution has recently been updated.
         *
         * \remark This must be atomic because the resolution can be updated
         *         from the game thread.
         */
        std::atomic_bool dirty_resolution;

        pimpl_Window(Window *parent):
                parent(parent) {
        }

        pimpl_Window(const pimpl_Window&) = delete;

        pimpl_Window(pimpl_Window&&) = delete;
    };
}
