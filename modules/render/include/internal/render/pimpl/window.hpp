/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/threading.hpp"

// module core
#include "argus/core.hpp"

// module render
#include "argus/render/renderer.hpp"

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

        pimpl_Window(Window &parent) {
        }

        pimpl_Window(const pimpl_Window&) = delete;

        pimpl_Window(pimpl_Window&&) = delete;
    };
}
