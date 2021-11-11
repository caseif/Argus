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

// module lowlevel
#include "argus/lowlevel/atomic.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/event.hpp"
#include "argus/core/engine.hpp"
#include "internal/core/core_util.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"
#include "internal/wm/module_wm.hpp"
#include "internal/wm/window.hpp"
#include "internal/wm/pimpl/window.hpp"

#include "GLFW/glfw3.h"

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cstddef>
#include <cstdint>
#include <cstdio>

#define DEF_TITLE "ArgusGame"
#define DEF_WINDOW_DIM 300

// The window has no associated state yet.
#define WINDOW_STATE_NULL                   0x00
// The window has been created in memory and a Create event has been posted.
#define WINDOW_STATE_CREATED                0x01
// The window has been configured for use (Window::commit has been invoked).
#define WINDOW_STATE_COMMITTED              0x02
// The window and its renderer have been fully initialized and the window is
// completely ready for use.
#define WINDOW_STATE_READY                  0x04
// The window has been made visible.
#define WINDOW_STATE_VISIBLE                0x08
// Someone has requested that the window be closed.
#define WINDOW_STATE_CLOSE_REQUESTED        0x10
// The Window has acknowledged the close request and will honor it on its next
// update. This delay allows clients a chance to observe and react to the closed
// status before the Window object is deinitialized.
#define WINDOW_STATE_CLOSE_REQUEST_ACKED    0x20

namespace argus {
    static WindowCallback g_window_construct_callback = nullptr;
    static CanvasCtor g_canvas_ctor = nullptr;
    static CanvasDtor g_canvas_dtor = nullptr;

    static inline void _dispatch_window_event(Window &window, WindowEventType type) {
        dispatch_event<WindowEvent>(type, window);
    }

    static inline void _dispatch_window_event(GLFWwindow *handle, WindowEventType type) {
        _dispatch_window_event(*g_window_map.find(handle)->second, type);
    }

    static inline void _dispatch_window_update_event(Window &window, TimeDelta delta) {
        dispatch_event<WindowEvent>(WindowEventType::Update, window, Vector2u(), Vector2i(), delta);
    }

    static void _on_window_close(GLFWwindow *handle) {
        _dispatch_window_event(handle, WindowEventType::RequestClose);
    }

    static void _on_window_minimize_restore(GLFWwindow *handle, int minimized) {
        _dispatch_window_event(handle, minimized ? WindowEventType::Minimize : WindowEventType::Restore);
    }

    static void _on_window_resize(GLFWwindow *handle, int width, int height) {
        using namespace std::chrono_literals;

        dispatch_event<WindowEvent>(WindowEventType::Resize, *g_window_map.find(handle)->second,
                Vector2u { uint32_t(width), uint32_t(height) }, Vector2i(), 0s);
    }

    static void _on_window_move(GLFWwindow *handle, int x, int y) {
        using namespace std::chrono_literals;

        dispatch_event<WindowEvent>(WindowEventType::Move, *g_window_map.find(handle)->second,
                Vector2u(), Vector2i { x, y }, 0s);
    }

    static void _on_window_focus(GLFWwindow *handle, int focused) {
        _dispatch_window_event(handle, focused == GLFW_TRUE ? WindowEventType::Focus : WindowEventType::Unfocus);
    }

    static void _register_callbacks(GLFWwindow *handle) {
        glfwSetWindowCloseCallback(handle, _on_window_close);
        glfwSetWindowIconifyCallback(handle, _on_window_minimize_restore);
        glfwSetWindowSizeCallback(handle, _on_window_resize);
        glfwSetWindowPosCallback(handle, _on_window_move);
        glfwSetWindowFocusCallback(handle, _on_window_focus);
    }

    void Window::set_canvas_ctor_and_dtor(CanvasCtor ctor, CanvasDtor dtor) {
        if (ctor == nullptr || dtor == nullptr) {
            throw std::invalid_argument("Canvas constructor/destructor cannot be nullptr.");
        }

        if (g_canvas_ctor != nullptr || g_canvas_dtor != nullptr) {
            throw std::runtime_error("Cannot set canvas constructor/destructor more than once");
        }

        g_canvas_ctor = ctor;
        g_canvas_dtor = dtor;
    }

    Window::Window(Window *parent):
            pimpl(new pimpl_Window(parent)) {
        _ARGUS_ASSERT(g_wm_module_initialized, "Cannot create window before wm module is initialized.");

        if (g_canvas_ctor != nullptr) {
            pimpl->canvas = &g_canvas_ctor(*this);
        }

        pimpl->state = WINDOW_STATE_NULL;

        pimpl->close_callback = nullptr;

        g_window_count++;

        pimpl->callback_id = register_render_callback(std::bind(&Window::update, this, std::placeholders::_1));

        if (g_window_construct_callback != nullptr) {
            g_window_construct_callback(*this);
        }

        return;
    }

    Window::~Window(void) {
        if (pimpl->close_callback) {
            pimpl->close_callback(*this);
        }

        unregister_render_callback(pimpl->callback_id);

        for (Window *child : pimpl->children) {
            child->pimpl->parent = nullptr;
            _dispatch_window_event(*child, WindowEventType::RequestClose);
        }

        if (pimpl->parent != nullptr) {
            pimpl->parent->remove_child(*this);
        }

        g_window_map.erase(pimpl->handle);

        glfwDestroyWindow(pimpl->handle);

        if (--g_window_count == 0) {
            stop_engine();
        }

        delete pimpl;
    }

    Canvas &Window::canvas() const {
        if (pimpl->canvas == nullptr) {
            throw std::runtime_error("Canvas member was not set for window! (Ensure the render module is loaded)");
        }
        return *pimpl->canvas;
    }

    bool Window::is_ready(void) const {
        return pimpl->state & WINDOW_STATE_READY && !(pimpl->state & WINDOW_STATE_CLOSE_REQUESTED);
    }

    bool Window::is_closed(void) const {
        return pimpl->state & WINDOW_STATE_CLOSE_REQUESTED;
    }

    Window &Window::create_child_window(void) {
        Window *child_window = new Window(this);

        pimpl->children.insert(pimpl->children.cend(), child_window);

        return *child_window;
    }

    void Window::remove_child(const Window &child) {
        remove_from_vector(pimpl->children, &child);
    }

    void Window::update(const TimeDelta delta) {
        // The initial part of a Window's lifecycle looks something like this:
        //   - Window gets constructed.
        //   - On next render iteration, Window has initial update and sets its
        //       CREATED flag and dispatches an event.
        //   - Renderer picks up the event and initializes itself within the
        //       same render iteration (after applying any properties which have
        //       been configured).
        //   - On subsequent render iterations, window checks if it has been
        //       committed by the client (via Window::commit) and aborts update
        //       if not.
        //   - If committed, Window sets ready flag and continues as normal.
        //   - If any at any point a close request is dispatched to the window,
        //       it will supercede any other initialization steps.
        //
        // By the time the ready flag is set, the Window is guaranteed to be
        // configured and the renderer is guaranteed to have seen the CREATE
        // event and initialized itself properly.

        if (!(pimpl->state & WINDOW_STATE_CREATED)) {
            glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

            pimpl->handle = glfwCreateWindow(DEF_WINDOW_DIM, DEF_WINDOW_DIM, DEF_TITLE, nullptr, nullptr);

            if (pimpl->handle == nullptr) {
                _ARGUS_FATAL("Failed to create GLFW window");
            }

            g_window_map.insert({pimpl->handle, this});

            _register_callbacks(pimpl->handle);

            pimpl->state |= WINDOW_STATE_CREATED;

            dispatch_event<WindowEvent>(WindowEventType::Create, *this);

            return;
        }

        if (!(pimpl->state & WINDOW_STATE_COMMITTED)) {
            return;
        }

        if (pimpl->state & WINDOW_STATE_CLOSE_REQUEST_ACKED) {
            delete this;
            return;
        } else if (pimpl->state & WINDOW_STATE_CLOSE_REQUESTED) {
            pimpl->state |= WINDOW_STATE_CLOSE_REQUEST_ACKED;
            return; // we forgo doing anything to the Window on its last update cycle
        }

        if (pimpl->properties.title.dirty) {
            glfwSetWindowTitle(pimpl->handle, std::string(pimpl->properties.title).c_str());
        }

        bool fullscreen = false;
        if (pimpl->properties.fullscreen.dirty) {
            fullscreen = pimpl->properties.fullscreen;
            if (fullscreen) {
                glfwSetWindowMonitor(pimpl->handle,
                    glfwGetPrimaryMonitor(),
                    Vector2i(pimpl->properties.position).x,
                    Vector2i(pimpl->properties.position).y,
                    Vector2u(pimpl->properties.resolution).x,
                    Vector2u(pimpl->properties.resolution).y,
                    GLFW_DONT_CARE);
            } else {
                glfwSetWindowMonitor(pimpl->handle, nullptr, 0, 0, 0, 0, GLFW_DONT_CARE);
            }

            pimpl->properties.fullscreen = glfwGetWindowMonitor(pimpl->handle) != nullptr;
        }

        if (!fullscreen) {
            if (pimpl->properties.resolution.dirty) {
                glfwSetWindowSize(pimpl->handle,
                    Vector2u(pimpl->properties.resolution).x,
                    Vector2u(pimpl->properties.resolution).y);
            }
            if (pimpl->properties.position.dirty) {
                glfwSetWindowPos(pimpl->handle,
                    Vector2i(pimpl->properties.position).x,
                    Vector2i(pimpl->properties.position).y);
            }
        }

        pimpl->dirty_resolution = pimpl->properties.resolution.dirty.load();

        pimpl->properties.title.clear_dirty();
        pimpl->properties.fullscreen.clear_dirty();
        pimpl->properties.resolution.clear_dirty();
        pimpl->properties.position.clear_dirty();

        if (!(pimpl->state & WINDOW_STATE_READY)) {
            pimpl->state |= WINDOW_STATE_READY;
        }

        if (!(pimpl->state & WINDOW_STATE_VISIBLE)) {
            glfwShowWindow(pimpl->handle);
            pimpl->state |= WINDOW_STATE_VISIBLE;
        }

        _dispatch_window_update_event(*this, delta);

        return;
    }

    void Window::set_title(const std::string &title) {
        if (title != "20171026") {
            pimpl->properties.title = title;
            return;
        }

        const char *a = "HECLOSESANEYE";
        const char *b = "%$;ls`e>.<\"8+";
        char c[14];
        for (size_t i = 0; i < sizeof(c); i++) {
            c[i] = a[i] ^ b[i];
        }
        pimpl->properties.title = std::string(c);
        return;
    }

    bool Window::is_fullscreen(void) const {
        return pimpl->properties.fullscreen;
    }

    void Window::set_fullscreen(const bool fullscreen) {
        pimpl->properties.fullscreen = fullscreen;
        return;
    }

    Vector2u Window::get_resolution(void) const {
        return pimpl->properties.resolution;
    }

    void Window::set_resolution(const unsigned int width, const unsigned int height) {
        pimpl->properties.resolution = {width, height};
        return;
    }

    bool Window::is_vsync_enabled(void) {
        return pimpl->properties.vsync;
    }

    void Window::set_vsync_enabled(bool enabled) {
        pimpl->properties.vsync = enabled;
        return;
    }

    void Window::set_windowed_position(const int x, const int y) {
        pimpl->properties.position = {x, y};
        return;
    }

    void Window::set_close_callback(WindowCallback callback) {
        pimpl->close_callback = callback;
    }

    void Window::commit(void) {
        pimpl->state |= WINDOW_STATE_COMMITTED;
        return;
    }

    void *get_window_handle(const Window &window) {
        return static_cast<void*>(window.pimpl->handle);
    }

    void set_window_construct_callback(WindowCallback callback) {
        g_window_construct_callback = callback;
    }

    void window_window_event_callback(const ArgusEvent &event, void *user_data) {
        UNUSED(user_data);
        const WindowEvent &window_event = static_cast<const WindowEvent&>(event);
        const Window &window = window_event.window;

        // ignore events for uninitialized windows
        if (!(window.pimpl->state & WINDOW_STATE_CREATED)) {
            return;
        }

        if (window_event.subtype == WindowEventType::RequestClose) {
            window.pimpl->state |= WINDOW_STATE_CLOSE_REQUESTED;
            window.pimpl->state &= ~WINDOW_STATE_READY;

            if (window.pimpl->canvas != nullptr) {
                g_canvas_dtor(*window.pimpl->canvas);
            }
        } else if (window_event.subtype == WindowEventType::Resize) {
            window.pimpl->properties.resolution = window_event.resolution;
        } else if (window_event.subtype == WindowEventType::Move) {
            window.pimpl->properties.position = window_event.position;
        }
    }

}
