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

#include "argus/lowlevel/atomic.hpp"
#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"
#include "argus/lowlevel/vector.hpp"

#include "argus/core/client_properties.hpp"
#include "argus/core/event.hpp"
#include "argus/core/engine.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"
#include "internal/wm/display.hpp"
#include "internal/wm/module_wm.hpp"
#include "internal/wm/window.hpp"
#include "internal/wm/pimpl/display.hpp"
#include "internal/wm/pimpl/window.hpp"

#include "GLFW/glfw3.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>

#include <climits>

#define DEF_WINDOW_DIM 300

// The window has no associated state yet.
#define WINDOW_STATE_NULL                   0x00U
// The window has been created in memory and a Create event has been posted.
#define WINDOW_STATE_CREATED                0x01U
// The window has been configured for use (Window::commit has been invoked).
#define WINDOW_STATE_COMMITTED              0x02U
// The window and its renderer have been fully initialized and the window is
// completely ready for use.
#define WINDOW_STATE_READY                  0x04U
// The window has been made visible.
#define WINDOW_STATE_VISIBLE                0x08U
// Someone has requested that the window be closed.
#define WINDOW_STATE_CLOSE_REQUESTED        0x10U
// The Window has acknowledged the close request and will honor it on its next
// update. This delay allows clients a chance to observe and react to the closed
// status before the Window object is deinitialized.
#define WINDOW_STATE_CLOSE_REQUEST_ACKED    0x20U

namespace argus {
    static WindowCallback g_window_construct_callback = nullptr;
    static CanvasCtor g_canvas_ctor = nullptr;
    static CanvasDtor g_canvas_dtor = nullptr;

    static std::vector<Window *> g_windows;

    static inline void _dispatch_window_event(Window &window, WindowEventType type) {
        dispatch_event<WindowEvent>(type, window);
    }

    static inline void _dispatch_window_event(GLFWwindow *handle, WindowEventType type) {
        _dispatch_window_event(*g_window_handle_map.find(handle)->second, type);
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

        dispatch_event<WindowEvent>(WindowEventType::Resize, *g_window_handle_map.find(handle)->second,
                Vector2u{uint32_t(width), uint32_t(height)}, Vector2i(), 0s);
    }

    static void _on_window_move(GLFWwindow *handle, int x, int y) {
        using namespace std::chrono_literals;

        dispatch_event<WindowEvent>(WindowEventType::Move, *g_window_handle_map.find(handle)->second,
                Vector2u(), Vector2i{x, y}, 0s);
    }

    static void _on_window_focus(GLFWwindow *handle, int focused) {
        _dispatch_window_event(handle, focused == GLFW_TRUE ? WindowEventType::Focus : WindowEventType::Unfocus);
    }

    static void _on_window_content_scale_change(GLFWwindow *handle, float x, float y) {
        Window *window = get_window_from_handle(handle);
        window->pimpl->content_scale.x = x;
        window->pimpl->content_scale.y = y;
    }

    static void _register_callbacks(GLFWwindow *handle) {
        glfwSetWindowCloseCallback(handle, _on_window_close);
        glfwSetWindowIconifyCallback(handle, _on_window_minimize_restore);
        glfwSetWindowSizeCallback(handle, _on_window_resize);
        glfwSetWindowPosCallback(handle, _on_window_move);
        glfwSetWindowFocusCallback(handle, _on_window_focus);
        glfwSetWindowContentScaleCallback(handle, _on_window_content_scale_change);
    }

    static void _reap_window(Window &window) {
        unregister_render_callback(window.pimpl->callback_id);
        run_on_game_thread([&window] { delete &window; });
    }

    void reap_windows(void) {
        auto it = g_windows.begin();

        while (it != g_windows.end()) {
            Window &win = **it;
            if (win.pimpl->state & WINDOW_STATE_CLOSE_REQUEST_ACKED) {
                _reap_window(win);
                it = g_windows.erase(it);
            } else {
                ++it;
            }
        }
    }

    Window *get_window(const std::string &id) {
        auto window_it = g_window_id_map.find(id);
        return window_it != g_window_id_map.end() ? g_window_id_map.find(id)->second : nullptr;
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

    Window &Window::create(const std::string &id, Window *parent) {
        auto *window = new Window(id, parent);
        g_windows.push_back(window);
        return *window;
    }

    Window::Window(const std::string &id, Window *parent) :
            pimpl(new pimpl_Window(id, parent)) {
        affirm_precond(g_wm_module_initialized, "Cannot create window before wm module is initialized.");

        if (g_canvas_ctor != nullptr) {
            pimpl->canvas = &g_canvas_ctor(*this);
        } else {
            Logger::default_logger().warn("No canvas callbacks were set - new window will not have associated canvas!");
        }

        pimpl->state = WINDOW_STATE_NULL;

        pimpl->close_callback = nullptr;

        g_window_id_map.insert({id, this});

        g_window_count++;

        pimpl->callback_id = register_render_callback(std::bind(&Window::update, this, std::placeholders::_1));

        if (g_window_construct_callback != nullptr) {
            g_window_construct_callback(*this);
        }

        return;
    }

    Window::Window(Window &&rhs) : pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Window::~Window(void) {
        if (pimpl == nullptr) {
            return;
        }

        if (pimpl->close_callback) {
            pimpl->close_callback(*this);
        }

        glfwDestroyWindow(pimpl->handle);

        for (Window *child : pimpl->children) {
            child->pimpl->parent = nullptr;
            _dispatch_window_event(*child, WindowEventType::RequestClose);
        }

        if (pimpl->parent != nullptr) {
            pimpl->parent->remove_child(*this);
        }

        g_window_id_map.erase(pimpl->id);
        g_window_handle_map.erase(pimpl->handle);

        g_window_count--;

        delete pimpl;
    }

    const std::string &Window::get_id(void) const {
        return pimpl->id;
    }

    Canvas &Window::get_canvas(void) const {
        if (pimpl->canvas == nullptr) {
            throw std::runtime_error("Canvas member was not set for window! (Ensure the render module is loaded)");
        }
        return *pimpl->canvas;
    }

    bool Window::is_created(void) const {
        return pimpl->state & WINDOW_STATE_CREATED;
    }

    bool Window::is_ready(void) const {
        return pimpl->state & WINDOW_STATE_READY && !(pimpl->state & WINDOW_STATE_CLOSE_REQUESTED);
    }

    bool Window::is_closed(void) const {
        return pimpl->state & WINDOW_STATE_CLOSE_REQUESTED;
    }

    Window &Window::create_child_window(const std::string &id) {
        Window *child_window = new Window(id, this);

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
            glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
            glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
            glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_TRUE);

            pimpl->handle = glfwCreateWindow(DEF_WINDOW_DIM, DEF_WINDOW_DIM, get_client_name().c_str(), nullptr,
                    nullptr);

            if (pimpl->handle == nullptr) {
                Logger::default_logger().fatal("Failed to create GLFW window");
            }

            g_window_handle_map.insert({pimpl->handle, this});

            _register_callbacks(pimpl->handle);

            pimpl->state |= WINDOW_STATE_CREATED;

            glfwGetWindowContentScale(pimpl->handle, &pimpl->content_scale.x, &pimpl->content_scale.y);

            dispatch_event<WindowEvent>(WindowEventType::Create, *this);

            return;
        }

        if (!(pimpl->state & WINDOW_STATE_COMMITTED)) {
            return;
        }

        if ((pimpl->state & WINDOW_STATE_CLOSE_REQUESTED)) {
            // don't acknowledge close until all references from events are released
            if (pimpl->refcount.load() <= 0) {
                pimpl->state |= WINDOW_STATE_CLOSE_REQUEST_ACKED;
            }
            return; // we forego doing anything else after a close request has been sent
        }

        auto title = pimpl->properties.title.read();
        auto fullscreen = pimpl->properties.fullscreen.read();
        auto display = pimpl->properties.display.read();
        auto custom_display_mode = pimpl->properties.custom_display_mode.read();
        auto display_mode = pimpl->properties.display_mode.read();
        auto windowed_res = pimpl->properties.windowed_resolution.read();
        auto position = pimpl->properties.position.read();
        auto mouse_capture = pimpl->properties.mouse_capture.read();
        auto mouse_visible = pimpl->properties.mouse_visible.read();
        auto mouse_raw_input = pimpl->properties.mouse_raw_input.read();

        if (title.dirty) {
            glfwSetWindowTitle(pimpl->handle, title->c_str());
        }

        if (fullscreen.dirty || (fullscreen && display.dirty)) {
            if (fullscreen) {
                // switch to fullscreen mode (or switch fullscreen monitor)

                DisplayMode cur_display_mode;
                if (custom_display_mode) {
                    cur_display_mode = display_mode;
                } else {
                    cur_display_mode = get_display_mode();
                }

                affirm_precond(cur_display_mode.resolution.x <= INT_MAX && cur_display_mode.resolution.y <= INT_MAX,
                        "Current display mode fullscreen resolution is too large");

                glfwSetWindowMonitor(pimpl->handle,
                        get_display_affinity().pimpl->handle,
                        0,
                        0,
                        int(cur_display_mode.resolution.x),
                        int(cur_display_mode.resolution.y),
                        cur_display_mode.refresh_rate);

                pimpl->cur_resolution = cur_display_mode.resolution;
                pimpl->cur_refresh_rate = cur_display_mode.refresh_rate;
            } else {
                affirm_precond(windowed_res->x <= INT_MAX && windowed_res->y <= INT_MAX,
                        "Current windowed resolution is too large");

                // switch to windowed mode
                glfwSetWindowMonitor(pimpl->handle,
                        nullptr,
                        position->x,
                        position->y,
                        int32_t(windowed_res->x),
                        int32_t(windowed_res->y),
                        GLFW_DONT_CARE);
                pimpl->cur_resolution = windowed_res;
            }
        } else if (fullscreen && (custom_display_mode.dirty || display_mode.dirty)) {
            // switch fullscreen display mode

            DisplayMode cur_display_mode;
            if (custom_display_mode) {
                cur_display_mode = display_mode;
            } else {
                cur_display_mode = get_display_mode();
            }

            affirm_precond(cur_display_mode.resolution.x <= INT_MAX && cur_display_mode.resolution.y <= INT_MAX,
                    "Current display mode fullscreen resolution is too large");

            if (cur_display_mode.refresh_rate != pimpl->cur_refresh_rate) {
                glfwSetWindowMonitor(pimpl->handle,
                        get_display_affinity().pimpl->handle,
                        0,
                        0,
                        int32_t(cur_display_mode.resolution.x),
                        int32_t(cur_display_mode.resolution.y),
                        cur_display_mode.refresh_rate);

                pimpl->cur_resolution = cur_display_mode.resolution;
                pimpl->cur_refresh_rate = cur_display_mode.refresh_rate;
            } else {
                glfwSetWindowSize(pimpl->handle,
                        int32_t(cur_display_mode.resolution.x),
                        int32_t(cur_display_mode.resolution.y));

                pimpl->cur_resolution = cur_display_mode.resolution;
            }
        } else if (!fullscreen) {
            // update windowed positon and/or resolution

            if (windowed_res.dirty) {
                affirm_precond(windowed_res->x <= INT_MAX && windowed_res->y <= INT_MAX,
                        "Current windowed resolution is too large");

                glfwSetWindowSize(pimpl->handle,
                        int32_t(windowed_res->x),
                        int32_t(windowed_res->y));

                pimpl->cur_resolution = windowed_res;
            }

            if (position.dirty) {
                glfwSetWindowPos(pimpl->handle,
                        position->x,
                        position->y);
            }
        }

        if (mouse_capture.dirty || mouse_visible.dirty) {
            glfwSetInputMode(pimpl->handle, GLFW_CURSOR, mouse_capture
                                                         ? GLFW_CURSOR_DISABLED
                                                         : mouse_visible
                                                           ? GLFW_CURSOR_NORMAL
                                                           : GLFW_CURSOR_HIDDEN);
        }

        if (mouse_raw_input.dirty) {
            glfwSetInputMode(pimpl->handle, GLFW_RAW_MOUSE_MOTION, mouse_raw_input ? GLFW_TRUE : GLFW_FALSE);
        }

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
        return pimpl->properties.fullscreen.peek();
    }

    void Window::set_fullscreen(const bool fullscreen) {
        pimpl->properties.fullscreen = fullscreen;
        return;
    }

    ValueAndDirtyFlag<Vector2u> Window::get_resolution(void) {
        return pimpl->cur_resolution.read();
    }

    Vector2u Window::peek_resolution(void) const {
        return pimpl->cur_resolution.peek();
    }

    Vector2u Window::get_windowed_resolution(void) const {
        return pimpl->properties.windowed_resolution.peek();
    }

    void Window::set_windowed_resolution(unsigned int width, unsigned int height) {
        pimpl->properties.windowed_resolution = {width, height};
        return;
    }

    void Window::set_windowed_resolution(const Vector2u &resolution) {
        pimpl->properties.windowed_resolution = {resolution.x, resolution.y};
        return;
    }

    ValueAndDirtyFlag<bool> Window::is_vsync_enabled(void) const {
        return pimpl->properties.vsync.read();
    }

    void Window::set_vsync_enabled(bool enabled) {
        pimpl->properties.vsync = enabled;
        return;
    }

    void Window::set_windowed_position(int x, int y) {
        pimpl->properties.position = {x, y};
        return;
    }

    void Window::set_windowed_position(const Vector2i &position) {
        pimpl->properties.position = {position.x, position.y};
        return;
    }

    const Display &Window::get_display_affinity(void) const {
        const Display *display = pimpl->properties.display.peek();
        if (display != nullptr) {
            auto *found = get_display_from_handle(display->pimpl->handle);
            if (found != nullptr) {
                return *found;
            }
        }

        auto *primary = get_display_from_handle(glfwGetPrimaryMonitor());

        if (primary == nullptr) {
            throw std::runtime_error("No available displays!");
        }

        return *primary;
    }

    void Window::set_display_affinity(const Display &display) {
        auto *cur_display = pimpl->properties.display.peek();
        if (cur_display != nullptr && display.pimpl->handle == cur_display->pimpl->handle) {
            return;
        }

        auto *found = get_display_from_handle(display.pimpl->handle);
        if (found == nullptr) {
            return;
        }

        pimpl->properties.display = found;
        // reset display mode since it's not necessarily valid on the new display
        pimpl->properties.custom_display_mode = false;
    }

    DisplayMode Window::get_display_mode(void) const {
        if (pimpl->properties.custom_display_mode.peek()) {
            return pimpl->properties.display_mode.peek();
        } else {
            return get_display_affinity().get_display_modes().back();
        }
    }

    void Window::set_display_mode(DisplayMode mode) {
        pimpl->properties.custom_display_mode = true;
        pimpl->properties.display_mode = mode;
    }


    bool Window::is_mouse_captured(void) {
        return pimpl->properties.mouse_capture.peek();
    }

    void Window::set_mouse_captured(bool captured) {
        pimpl->properties.mouse_capture = captured;
    }

    bool Window::is_mouse_visible(void) {
        return pimpl->properties.mouse_visible.peek();
    }

    void Window::set_mouse_visible(bool visible) {
        pimpl->properties.mouse_visible = visible;
    }

    bool Window::is_mouse_raw_input(void) {
        return pimpl->properties.mouse_raw_input.peek();
    }

    void Window::set_mouse_raw_input(bool raw_input) {
        pimpl->properties.mouse_raw_input = raw_input;
    }

    Vector2f Window::get_content_scale(void) const {
        return pimpl->content_scale;
    }

    void Window::set_close_callback(WindowCallback callback) {
        pimpl->close_callback = callback;
    }

    void Window::commit(void) {
        pimpl->state |= WINDOW_STATE_COMMITTED;
        return;
    }

    void Window::request_close(void) {
        _dispatch_window_event(*this, WindowEventType::RequestClose);
    }

    void *get_window_handle(const Window &window) {
        return static_cast<void *>(window.pimpl->handle);
    }

    Window *get_window_from_handle(const void *handle) {
        auto it = g_window_handle_map.find(static_cast<GLFWwindow *>(const_cast<void *>(handle)));
        if (it == g_window_handle_map.end()) {
            return nullptr;
        }

        return it->second;
    }

    void set_window_construct_callback(WindowCallback callback) {
        g_window_construct_callback = callback;
    }

    void window_window_event_callback(const WindowEvent &event, void *user_data) {
        UNUSED(user_data);
        const Window &window = event.window;

        // ignore events for uninitialized windows
        if (!(window.pimpl->state & WINDOW_STATE_CREATED)) {
            return;
        }

        if (event.subtype == WindowEventType::RequestClose) {
            window.pimpl->state |= WINDOW_STATE_CLOSE_REQUESTED;
            window.pimpl->state &= ~WINDOW_STATE_READY;

            if (window.pimpl->canvas != nullptr) {
                g_canvas_dtor(*window.pimpl->canvas);
            }
        } else if (event.subtype == WindowEventType::Resize) {
            window.pimpl->cur_resolution = event.resolution;
        } else if (event.subtype == WindowEventType::Move) {
            if (glfwGetWindowMonitor(window.pimpl->handle) == nullptr) {
                window.pimpl->properties.position.set_quietly(event.position);
            }
        }
    }
}
