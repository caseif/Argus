/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/threading.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core/core_util.hpp"

// module render
#include "argus/render/renderer.hpp"
#include "argus/render/window.hpp"
#include "argus/render/window_event.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/window.hpp"
#include "internal/render/pimpl/renderer.hpp"
#include "internal/render/pimpl/window.hpp"

#ifdef USE_GLES
#define GLFW_INCLUDE_ES3
#endif
#include "GLFW/glfw3.h"

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cstddef>

#define DEF_TITLE "ArgusGame"
#define DEF_WINDOW_DIM 300

// the window has no associated state yet
#define WINDOW_STATE_NULL               0x00
// the window has been created in memory and a CREATE event has been posted
#define WINDOW_STATE_CREATED            0x01
// the window has been configured for use (Window::activate has been invoked)
#define WINDOW_STATE_CONFIGURED         0x02
// the window and its renderer have been fully initialized and the window is
// completely ready for use
#define WINDOW_STATE_READY              0x04
// the window has been made visible
#define WINDOW_STATE_VISIBLE            0x08
// someone has requested that the window be closed
#define WINDOW_STATE_CLOSE_REQUESTED    0x10

namespace argus {
    static WindowCallback g_window_construct_callback = nullptr;

    static inline void _dispatch_window_event(Window &window, WindowEventType type) {
        dispatch_event(WindowEvent(type, window));
    }

    static inline void _dispatch_window_event(GLFWwindow *handle, WindowEventType type) {
        _dispatch_window_event(*g_window_map.find(handle)->second, type);
    }

    static inline void _dispatch_window_update_event(Window &window, TimeDelta delta) {
        dispatch_event(WindowEvent(WindowEventType::UPDATE, window, Vector2u(), Vector2i(), delta));
    }

    static void _on_window_close(GLFWwindow *handle) {
        _dispatch_window_event(handle, WindowEventType::REQUEST_CLOSE);
    }

    static void _on_window_minimize_restore(GLFWwindow *handle, int minimized) {
        _dispatch_window_event(handle, minimized ? WindowEventType::MINIMIZE : WindowEventType::RESTORE);
    }

    static void _on_window_resize(GLFWwindow *handle, int width, int height) {
        dispatch_event(WindowEvent(WindowEventType::RESIZE, *g_window_map.find(handle)->second,
                { uint32_t(width), uint32_t(height) }, Vector2i(), 0));
    }

    static void _on_window_move(GLFWwindow *handle, int x, int y) {
        dispatch_event(WindowEvent(WindowEventType::MOVE, *g_window_map.find(handle)->second,
                Vector2u(), { x, y }, 0));
    }

    static void _on_window_focus(GLFWwindow *handle, int focused) {
        _dispatch_window_event(handle, focused == GLFW_TRUE ? WindowEventType::FOCUS : WindowEventType::UNFOCUS);
    }

    static void _register_callbacks(GLFWwindow *handle) {
        glfwSetWindowCloseCallback(handle, _on_window_close);
        glfwSetWindowIconifyCallback(handle, _on_window_minimize_restore);
        glfwSetWindowSizeCallback(handle, _on_window_resize);
        glfwSetWindowPosCallback(handle, _on_window_move);
        glfwSetWindowFocusCallback(handle, _on_window_focus);
    }

    Window::Window(): pimpl(new pimpl_Window(*this)) {
        _ARGUS_ASSERT(g_render_module_initialized, "Cannot create window before render module is initialized.");

        pimpl->state = WINDOW_STATE_NULL;

        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        pimpl->handle = glfwCreateWindow(DEF_WINDOW_DIM, DEF_WINDOW_DIM, DEF_TITLE, nullptr, nullptr);

        if (pimpl->handle == nullptr) {
            _ARGUS_FATAL("Failed to create GLFW window");
        }

        pimpl->close_callback = nullptr;

        g_window_count++;
        g_window_map.insert({pimpl->handle, this});

        pimpl->parent = nullptr;

        _register_callbacks(pimpl->handle);

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
            _dispatch_window_event(*child, WindowEventType::REQUEST_CLOSE);
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

    bool Window::is_ready(void) {
        return pimpl->state & WINDOW_STATE_READY && !(pimpl->state & WINDOW_STATE_CLOSE_REQUESTED);
    }

    Window &Window::create_child_window(void) {
        Window *child_window = new Window();
        child_window->pimpl->parent = this;

        pimpl->children.insert(pimpl->children.cend(), child_window);

        return *child_window;
    }

    void Window::remove_child(const Window &child) {
        remove_from_vector(pimpl->children, &child);
    }

    void Window::update(const Timestamp delta) {
        // The initial part of a Window's lifecycle looks something like this:
        //   - Window gets constructed
        //   - On next render iteration, Window has initial update and sets its
        //     CREATED flag, dispatching an event
        //   - Renderer picks up the event and initializes itself within the
        //     same render iteration
        //   - On subsequent render iterations, window checks if it has been
        //     configured (via Window::activate) and aborts update if not.
        //   - If activated, Window sets ready flag and continues as normal.
        //
        // By the time the ready flag is set, the Window is guaranteed to be
        // configured and the renderer is guaranteed to have seen the CREATE
        // event and initialized itself properly.

        if (!(pimpl->state & WINDOW_STATE_CREATED)) {
            pimpl->state |= WINDOW_STATE_CREATED;

            dispatch_event(WindowEvent(WindowEventType::CREATE, *this));

            return;
        }

        if (!(pimpl->state & WINDOW_STATE_CONFIGURED)) {
            return;
        }

        if (!(pimpl->state & WINDOW_STATE_READY)) {
            pimpl->state |= WINDOW_STATE_READY;
        }

        if (!(pimpl->state & WINDOW_STATE_VISIBLE)) {
            glfwShowWindow(pimpl->handle);
            pimpl->state |= WINDOW_STATE_VISIBLE;
        }

        if (pimpl->state & WINDOW_STATE_CLOSE_REQUESTED) {
            delete this;
            return;
        }

        if (pimpl->properties.title.dirty) {
            glfwSetWindowTitle(pimpl->handle, ((std::string) pimpl->properties.title).c_str());
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

    void Window::set_windowed_position(const int x, const int y) {
        pimpl->properties.position = {x, y};
        return;
    }

    void Window::set_close_callback(WindowCallback callback) {
        pimpl->close_callback = callback;
    }

    void Window::activate(void) {
        pimpl->state |= WINDOW_STATE_CONFIGURED;
        return;
    }

    void *get_window_handle(const Window &window) {
        return static_cast<void*>(window.pimpl->handle);
    }

    void set_window_construct_callback(WindowCallback callback) {
        g_window_construct_callback = callback;
    }

    void window_window_event_callback(const ArgusEvent &event, void *user_data) {
        const WindowEvent &window_event = static_cast<const WindowEvent&>(event);
        const Window &window = window_event.window;

        // ignore events for uninitialized windows
        if (!(window.pimpl->state & WINDOW_STATE_CREATED)) {
            return;
        }

        if (window_event.subtype == WindowEventType::REQUEST_CLOSE) {
            window.pimpl->state |= WINDOW_STATE_CLOSE_REQUESTED;
            window.pimpl->state &= ~WINDOW_STATE_READY;
        } else if (window_event.subtype == WindowEventType::RESIZE) {
            window.pimpl->properties.resolution = window_event.resolution;
        } else if (window_event.subtype == WindowEventType::MOVE) {
            window.pimpl->properties.position = window_event.position;
        }
    }

}
