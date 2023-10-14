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
#include "argus/lowlevel/time.hpp"

#include "argus/scripting.hpp"

#include "argus/wm/display.hpp"

#include <functional>
#include <optional>
#include <string>

namespace argus {
    // forward declarations
    class Canvas;

    class Window;

    struct pimpl_Window;

    /**
     * \brief A callback which operates on a window-wise basis.
     */
    typedef std::function<void(Window &)> WindowCallback;

    /**
     * \brief A callback which constructs a Canvas associated with a given
     *        Window.
     */
    typedef std::function<Canvas &(Window &)> CanvasCtor;

    /**
     * \brief A callback which destructs and deallocates a Canvas.
     */
    typedef std::function<void(Canvas &)> CanvasDtor;

    enum class WindowCreationFlags {
        None                = 0x0,
        OpenGL              = 0x1,
        Vulkan              = 0x2,
        Metal               = 0x4,
        DirectX             = 0x8,
        WebGPU              = 0x10,

        GraphicsApiMask = int(WindowCreationFlags::OpenGL)
                |     int(WindowCreationFlags::Vulkan)
                |     int(WindowCreationFlags::Metal)
                |     int(WindowCreationFlags::DirectX)
                |     int(WindowCreationFlags::WebGPU),
    };

    inline WindowCreationFlags operator&(WindowCreationFlags a, WindowCreationFlags b) {
        using U = std::underlying_type_t<WindowCreationFlags>;
        return WindowCreationFlags(U(a) & U(b));
    }

    inline WindowCreationFlags operator|(WindowCreationFlags a, WindowCreationFlags b) {
        using U = std::underlying_type_t<WindowCreationFlags>;
        return WindowCreationFlags(U(a) | U(b));
    }

    /**
     * \brief Sets the window creation flags globally.
     *
     * \param flags The flags to be used for all created windows.
     */
    void set_window_creation_flags(WindowCreationFlags flags);

    /**
     * \brief Returns the Window with the specified ID. This will return nullptr
     *        if a Window with the specified ID does not exist at the time that
     *        the method is called.
     *
     * \return The Window with the specified ID.
     */
    Window *get_window(const std::string &id);

    /**
     * \brief Returns an opaque pointer to the underlying handle of a Window.
     *
     * \return An opaque pointer to the window handle.
     */
    void *get_window_handle(const Window &window);

    /**
     * \brief Returns a pointer to the underlying handle of a Window cast as a
     *        pointer to the given type.
     * 
     * \tparam T The pointer type to cast the window handle to.
     * \param window The window to get the handle of.
     * \return The underlying handle of the Window.
     */
    template<typename T>
    T *get_window_handle(const Window &window) {
        return static_cast<T *>(get_window_handle(window));
    }

    /**
     * \brief Looks up a Window based on its underlying handle.
     *
     * \return The Window with the provided handle, or nullptr if the handle
     *         pointer is not known to the engine.
     */
    Window *get_window_from_handle(const void *handle);

    /**
     * \brief Represents an individual window on the screen.
     *
     * \attention Not all platforms may support multiple windows.
     *
     * \sa Canvas
     */
    class Window : AutoCleanupable {
      private:

        /**
         * \brief Creates a new Window.
         *
         * \param id The unique identifier of the Window.
         * \param parent The Window which is parent to the new one, or
         *        `nullptr` if the window does not have a parent..
         *
         * \throw std::invalid_argument If the identifier is invalid or is
         *        already in use.
         *
         * \warning Not all platforms may support multiple
         *          \link Window Windows \endlink.
         *
         * \remark A Canvas will be implicitly created during construction
         *         of a Window.
         */
        Window(const std::string &id, Window *parent = nullptr);

        Window(Window &&) noexcept;

      public:
        pimpl_Window *pimpl;

        /**
         * \brief Sets the callbacks used to construct and destroy a Canvas
         *        during the lifecycle of a Window.
         *
         * The constructor will be invoked when the Window is constructed or
         * shortly thereafter, and the resulting Canvas will be associated
         * with the Window for the duration of its lifespan.
         *
         * The destructor will be invoked when a Window receives a close
         * request or shortly thereafter. The destructor should deinitialize
         * and deallocate the Canvas passed to it.
         *
         * \param ctor The constructor used to create Canvases.
         * \param dtor The destructor used to deinitialize and deallocate
         *        Canvases.
         *
         * \throw std::invalid_argument If either parameter is nullptr.
         * \throw std::runtime_error If the callbacks have already been set.
         */
        static void set_canvas_ctor_and_dtor(CanvasCtor ctor, CanvasDtor dtor);

        /**
         * \brief Creates a new Window.
         *
         * \param id The unique identifier of the Window.
         * \param parent The Window which is parent to the new one, or
         *        `nullptr` if the window does not have a parent..
         *
         * \throw std::invalid_argument If the identifier is invalid or is
         *        already in use.
         *
         * \warning Not all platforms may support multiple
         *          \link Window Windows \endlink.
         *
         * \remark A Canvas will be implicitly created during construction
         *         of a Window.
         */
        static Window &create(const std::string &id, Window *parent = nullptr);

        Window(const Window &) = delete;

        ~Window(void) override;

        /**
         * \brief Gets the unique identifier of the Window.
         *
         * \return The unique identifier of the Window.
         */
        const std::string &get_id(void) const;

        /**
         * \brief Gets the Canvas associated with the Window.
         *
         * \return The Canvas associated with the Window.
         *
         * \throw std::runtime_error If a Canvas has not been associated
         *        with the Window. This can occur if a renderer module has
         *        not been requested or if the renderer module is buggy.
         */
        Canvas &get_canvas(void) const;

        /**
         * \brief Gets whether the Window has been created by the windowing
         *        manager.
         *
         * \return Whether the Window has been created.
         */
        bool is_created(void) const;

        /**
         * \brief Gets whether the Window is ready for manipulation or interaction.
         *
         * \return Whether the Window is ready.
         */
        bool is_ready(void) const;

        /**
         * \brief Gets whether the Window is preparing to close.
         *
         * Once this is observed to return `true`, the Window object should
         * not be used again.
         *
         * \return Whether the Window is preparing to close.
         */
        bool is_closed(void) const;

        /**
         * \brief Creates a new window as a child of this one.
         *
         * \return The new child window.
         *
         * \note The child window will not be modal to the parent.
         */
        Window &create_child_window(const std::string &id);

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
         * \brief Gets the window's current resolution and whether it has
         *         changed since the last invocation of this function
         *
         * The internal dirty flag for the value will be copied to the
         * return object and then cleared in the Window object.
         *
         * \return The window's current resolution and dirty flag.
         *
         * \sa peek_resolution
         */
        ValueAndDirtyFlag<Vector2u> get_resolution(void);

        /**
         * \brief Gets the window's current resolution without affecting its
         *        internal dirty bit.
         *
         * If another function reads the window's resolution after this
         * function runs and the resolution has changed since the last time
         * get_resolution was called, it will see the change as not yet
         * having been acknowledged.
         *
         * \return The window's current resolution.
         *
         * \sa get_resolution
         */
        Vector2u peek_resolution(void) const;

        /**
         * \brief Gets the window's configured windowed resolution.
         *
         * \return The windowed resolution.
         */
        Vector2u get_windowed_resolution(void) const;

        /**
         * \brief Sets the window's windowed resolution.
         *
         * \param width The horizontal resolution.
         * \param height The vertical resolution.
         */
        void set_windowed_resolution(unsigned int width, unsigned int height);

        /**
         * \brief Sets the window's windowed resolution.
         *
         * \param resolution The new resolution of the window.
         */
        void set_windowed_resolution(const Vector2u &resolution);

        /**
         * \brief Gets whether the window has vertical synchronization
         *        (vsync) enabled and the state of its dirty flag.
         *
         * If the returned dirty flag is set, the vsync flag has been
         * changed since the last invocation of this function.
         *
         * \return Whether the window's vsync flag is set and its dirty
         *         flag.
         */
        ValueAndDirtyFlag<bool> is_vsync_enabled(void) const;

        /**
         * \brief Enabled or disabled vertical synchronization (vsync) for
         *        the window.
         *
         * \param enabled Whether vertical synchronization (vsync) should be
         *        enabled for the window.
         */
        void set_vsync_enabled(bool enabled);

        /**
         * \brief Sets the position of the window on the screen when in
         *        windowed mode.
         *
         * \param x The new X-coordinate of the window.
         * \param y The new Y-coordinate of the window.
         *
         * \warning This may not be supported on all platforms.
         */
        void set_windowed_position(int x, int y);

        /**
         * \brief Sets the position of the window on the screen when in
         *        windowed mode.
         *
         * \param position The new position of the window.
         *
         * \warning This may not be supported on all platforms.
         */
        void set_windowed_position(const Vector2i &position);

        /**
         * \brief Gets the Display this Window will attempt to show on in
         *        fullscreen mode.
         *
         * If this parameter has not been configured for the Window, it
         * will default to the primary display or otherwise the first
         * display available.
         *
         * \return The Display to show on.
         */
        const Display &get_display_affinity(void) const;

        /**
         * \brief Sets the Display this Window will attempt to show on in
         *        fullscreen mode.
         *
         * \param display The Display to show on.
         *
         * \sa Display::get_available_displays
         */
        void set_display_affinity(const Display &display);

        /**
         * \brief Gets the DisplayMode used by the Window while in
         *        fullscreen mode.
         *
         * If this parameter has not been configured for the Window, it will
         * default to the "best" available mode for its Display.
         *
         * The display mode controls parameters such as resolution, refresh
         * rate, and color depth.
         *
         * \return The display mode of the Window.
         */
        DisplayMode get_display_mode(void) const;

        /**
         * \brief Sets the DisplayMode of this Window while in fullscreen
         *        mode.
         *
         * The display mode controls parameters such as resolution, refresh
         * rate, and color depth.
         *
         * \param mode The display mode of the window.
         *
         * \sa Display::get_display_modes
         */
        void set_display_mode(DisplayMode mode);

        /**
         * \brief Gets whether the mouse cursor should be captured by the
         *        window while it is focused.
         *
         * \return Whether the mouse cursor should be captured.
         */
        bool is_mouse_captured(void);

        /**
         * \brief Sets whether the mouse cursor should be captured by the
         *        window while it is focused.
         *
         * \param captured Whether the mouse cursor should be captured.
         */
        void set_mouse_captured(bool captured);

        /**
         * \brief Gets whether the mouse cursor is visible within the
         *        window.
         *
         * \return Whether the mouse cursor is visible.
         */
        bool is_mouse_visible(void);

        /**
         * \brief Sets whether the mouse cursor is visible within the
         *        window.
         *
         * \param visible Whether the mouse cursor is visible.
         */
        void set_mouse_visible(bool visible);

        /**
         * \brief Gets whether the raw input from the mouse should be used
         *        by the window.
         *
         * \return Whether the raw mouse input should be used.
         */
        bool is_mouse_raw_input(void);

        /**
         * \brief Sets whether the raw input from the mouse should be used
         *        by the window.
         *
         * \param raw_input Whether the raw mouse input should be used.
         */
        void set_mouse_raw_input(bool raw_input);

        /**
         * \brief Gets the content scale of the Window as reported by the
         *        window manager.
         * \return The content scale of the Window.
         */
        [[nodiscard]] Vector2f get_content_scale(void) const;

        /**
         * \brief Sets the WindowCallback to invoke upon this window being
         *        closed.
         *
         * \param callback The callback to be executed.
         */
        void set_close_callback(WindowCallback callback);

        /**
         * \brief Commits the window configuration, prompting the engine to
         *        create it.
         *
         * \note This function should be invoked only once.
         */
        void commit(void);

        /**
         * \brief Sends a close request to the window.
         *
         * This will inititate the process of closing the window, although this
         * process will not occur immediately.
         *
         * This function is thread-safe.
         */
        void request_close(void);
    };
}
