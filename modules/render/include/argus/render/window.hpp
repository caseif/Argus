/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module core
#include "argus/core.hpp"

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

            ~Window(void);

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
            void update(const Timestamp delta);

            /**
             * \brief Handles \link ArgusEvent events \endlink relating to a
             *        Window.
             *
             * \param event The passed ArgusEvent.
             * \param user_data A pointer to the Window to handle events for.
             */
            void event_callback(const ArgusEvent &event, void *user_data);

            /**
             * \brief Destroys this window.
             *
             * \warning This method destroys the Window object. No other methods
             *          should be invoked upon it after calling destroy().
             */
            void destroy(void);

            /**
             * \brief Creates a new window as a child of this one.
             *
             * \return The new child window.
             *
             * \note The child window will not be modal to the parent.
             */
            Window &create_child_window(void);

            /**
             * Gets this Window's associated Renderer.
             *
             * \return The Window's Renderer.
             */
            Renderer &get_renderer(void);

            /**
             * Sets the window title.
             *
             * \param title The new window title.
             */
            void set_title(const std::string &title);

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
