/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"

#include <functional>
#include <string>

namespace argus {
    // forward declarations
    class Renderer;
    class Window;

    struct pimpl_Window;

    /**
     * \brief A callback which operates on a window-wise basis.
     */
    typedef std::function<void(Window &window)> WindowCallback;

    /**
     * \brief Represents an individual window on the screen.
     *
     * \attention Not all platforms may support multiple windows.
     *
     * \sa Renderer
     */
    class Window {
        public:
            pimpl_Window *const pimpl;

            /**
             * \brief Creates a new Window.
             *
             * \return The created Window.
             *
             * \warning Not all platforms may support multiple
             *          \link Window Windows \endlink.
             *
             * \remark A Renderer will be implicitly created upon construction
             *         of a Window.
             */
            Window();

            Window(const Window&) = delete;

            Window(Window&&) = delete;

            ~Window(void);

            /**
             * \brief Gets whether the Window is ready for manipulation or interaction.
             *
             * \return Whether the Window is ready.
             */
            bool is_ready(void);

            /**
             * \brief Creates a new window as a child of this one.
             *
             * \return The new child window.
             *
             * \note The child window will not be modal to the parent.
             */
            Window &create_child_window(void);

            /**
             * \brief Removes the given Window from this Window's child list.
             *
             * \param child The child Window to remove.
             *
             * \attention This method does not alter the state of the child
             *         Window, which must be dissociated from its parent separately.
             */
            void remove_child(const Window &child);

            /**
             * \brief The primary update callback for a Window.
             *
             * \param delta The time in microseconds since the last frame.
             */
            void update(const TimeDelta delta);

            /**
             * Sets the window title.
             *
             * \param title The new window title.
             */
            void set_title(const std::string &title);

            /**
             * \brief Gets whether the window is currently in fullscreen mode.
             *
             * \return The window's fullscreen state.
             */
            bool is_fullscreen(void) const;

            /**
             * \brief Sets the fullscreen state of the window.
             *
             * Caution: This may not be supported on all platforms.
             *
             * \param fullscreen Whether the window is to be displayed in
             *        fullscreen.
             */
            void set_fullscreen(const bool fullscreen);

            /**
             * \brief Gets the window's current resolution.
             *
             * \return The window's resolution.
             */
            Vector2u get_resolution(void) const;

            /**
             * \brief Sets the resolution of the window when not in fullscreen
             *        mode.
             *
             * \param width The new width of the window.
             * \param height The new height of the window.
             *
             * \warning This may not be supported on all platforms.
             */
            void set_resolution(const unsigned int width, const unsigned int height);

            /**
             * \brief Sets the position of the window on the screen when in
             *        windowed mode.
             *
             * \param x The new X-coordinate of the window.
             * \param y The new Y-coordinate of the window.
             *
             * \warning This may not be supported on all platforms.
             */
            void set_windowed_position(const int x, const int y);

            /**
             * \brief Sets the WindowCallback to invoke upon this window being
             *        closed.
             *
             * \param callback The callback to be executed.
             */
            void set_close_callback(WindowCallback callback);

            /**
             * \brief Activates the window.
             *
             * \note This function should be invoked only once.
             */
            void activate(void);
    };
}
