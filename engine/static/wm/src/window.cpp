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
#include "argus/lowlevel/enum_ops.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"
#include "argus/lowlevel/collections.hpp"

#include "argus/core/client_properties.hpp"
#include "argus/core/event.hpp"
#include "argus/core/engine.hpp"

#include "argus/wm/api_util.hpp"
#include "argus/wm/display.hpp"
#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"
#include "internal/wm/display.hpp"
#include "internal/wm/module_wm.hpp"
#include "internal/wm/window.hpp"
#include "internal/wm/pimpl/display.hpp"
#include "internal/wm/pimpl/window.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include "SDL_events.h"
#pragma GCC diagnostic pop
#include "SDL_hints.h"
#include "SDL_version.h"
#include "SDL_video.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <shared_mutex>
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
    // maps window IDs to Window instance pointers
    std::map<std::string, Window *> g_window_id_map;
    // maps SDL window pointers to Window instance pointers
    std::map<SDL_Window *, Window *> g_window_handle_map;
    size_t g_window_count = 0;

    // mutex for window ID and handle maps
    static std::shared_mutex g_window_maps_mutex;

    static WindowCreationFlags g_window_flags = WindowCreationFlags::None;
    static WindowCallback g_window_construct_callback = nullptr;
    static CanvasCtor g_canvas_ctor = nullptr;
    static CanvasDtor g_canvas_dtor = nullptr;

    static std::vector<Window *> g_windows;

    static inline void _dispatch_window_event(Window &window, WindowEventType type) {
        dispatch_event<WindowEvent>(type, window);
    }

    [[maybe_unused]] static inline void _dispatch_window_update_event(Window &window, TimeDelta delta) {
        dispatch_event<WindowEvent>(WindowEventType::Update, window, Vector2u(), Vector2i(), delta);
    }

    static int _on_window_event(void *udata, SDL_Event *event) {
        using namespace std::chrono_literals;

        UNUSED(udata);

        SDL_Window *handle = SDL_GetWindowFromID(event->window.windowID);
        /*if (SDL_GetWindowID(handle) != event->window.windowID) {
            return 0;
        }*/

        g_window_maps_mutex.lock_shared();
        auto it = g_window_handle_map.find(handle);
        if (it == g_window_handle_map.end()) {
            return 0;
        }
        auto &window = *it->second;
        g_window_maps_mutex.unlock_shared();

        if (window.is_closed()) {
            return 0;
        }

        switch (event->window.event) {
            case SDL_WINDOWEVENT_MOVED:
                dispatch_event<WindowEvent>(WindowEventType::Move, window,
                        Vector2u(), Vector2i { event->window.data1, event->window.data2 }, 0s);
                break;
            case SDL_WINDOWEVENT_RESIZED:
                dispatch_event<WindowEvent>(WindowEventType::Resize, window,
                        Vector2u { uint32_t(event->window.data1), uint32_t(event->window.data2) }, Vector2i(), 0s);
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                _dispatch_window_event(window, WindowEventType::Minimize);
                break;
            case SDL_WINDOWEVENT_RESTORED:
                _dispatch_window_event(window, WindowEventType::Restore);
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                _dispatch_window_event(window, WindowEventType::Focus);
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                _dispatch_window_event(window, WindowEventType::Unfocus);
                break;
            case SDL_WINDOWEVENT_CLOSE:
                _dispatch_window_event(window, WindowEventType::RequestClose);
                break;
                //TODO: handle display scale changed event when we move to SDL 3
        }

        return 0;
    }

    static void _register_callbacks(SDL_Window *handle) {
        UNUSED(handle);
        //SDL_AddEventWatch(_on_window_event, handle);
    }

    void peek_sdl_window_events(void) {
        SDL_Event event;
        while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT) > 0) {
            _on_window_event(nullptr, &event);
        }
    }

    void set_window_creation_flags(WindowCreationFlags flags) {
        g_window_flags = flags;
    }

    static void _reap_window(Window &window) {
        unregister_render_callback(window.pimpl->callback_id);
        delete &window;
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

    void reset_window_displays(void) {
        std::shared_lock<std::shared_mutex> lock(g_window_maps_mutex);
        for (const auto &[handle, window] : g_window_handle_map) {
            if (window->is_closed()) {
                continue;
            }

            auto new_disp_index = SDL_GetWindowDisplayIndex(handle);
            if (new_disp_index < 0 || size_t(new_disp_index) >= Display::get_available_displays().size()) {
                Logger::default_logger().warn("Failed to query new display of window ID %s, "
                                              "things might not work correctly!", window->get_id().c_str());
                continue;
            }
            window->pimpl->properties.display.set_quietly(get_display_from_index(new_disp_index));
        }
    }

    Window *get_window(const std::string &id) {
        std::shared_lock<std::shared_mutex> lock(g_window_maps_mutex);
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

        {
            std::unique_lock<std::shared_mutex> lock(g_window_maps_mutex);
            g_window_id_map.insert({ id, this });
        }

        g_window_count++;

        pimpl->callback_id = register_render_callback([this](TimeDelta delta) { this->update(delta); }, Ordering::Early);

        if (g_window_construct_callback != nullptr) {
            g_window_construct_callback(*this);
        }

        return;
    }

    Window::Window(Window &&rhs) noexcept : pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Window::~Window(void) {
        if (pimpl == nullptr) {
            return;
        }

        {
            std::shared_lock<std::shared_mutex> lock(g_window_maps_mutex);
            g_window_id_map.erase(pimpl->id);
            g_window_handle_map.erase(pimpl->handle);
        }

        if (pimpl->close_callback) {
            pimpl->close_callback(*this);
        }

        SDL_DestroyWindow(pimpl->handle);

        for (Window *child : pimpl->children) {
            child->pimpl->parent = nullptr;
            _dispatch_window_event(*child, WindowEventType::RequestClose);
        }

        if (pimpl->parent != nullptr) {
            pimpl->parent->remove_child(*this);
        }

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
        using namespace argus::enum_ops;

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

        if ((pimpl->state & WINDOW_STATE_CLOSE_REQUESTED)) {
            // don't acknowledge close until all references from events are released
            auto rc = pimpl->refcount.load();
            if (rc <= 0) {
                pimpl->state |= WINDOW_STATE_CLOSE_REQUEST_ACKED;
            }
            return; // we forego doing anything else after a close request has been sent
        }

        if (!(pimpl->state & WINDOW_STATE_CREATED)) {
            SDL_WindowFlags sdl_flags = SDL_WindowFlags(
                    SDL_WINDOW_RESIZABLE
                    | SDL_WINDOW_INPUT_FOCUS
                    | SDL_WINDOW_HIDDEN
            );

            auto gfx_api_bits = g_window_flags & WindowCreationFlags::GraphicsApiMask;
            if ((gfx_api_bits & (int(gfx_api_bits) - 1)) != 0) {
                throw std::invalid_argument("Only one graphics API may be set during window creation");
            }

            if ((g_window_flags & WindowCreationFlags::OpenGL) != 0) {
                sdl_flags = SDL_WindowFlags(sdl_flags | SDL_WINDOW_OPENGL);
            } else if ((g_window_flags & WindowCreationFlags::Vulkan) != 0) {
                sdl_flags = SDL_WindowFlags(sdl_flags | SDL_WINDOW_VULKAN);
            } else if ((g_window_flags & WindowCreationFlags::Metal) != 0) {
                #ifdef __APPLE__
                #if SDL_VERSION_ATLEAST(2, 0, 14)
                sdl_flags = SDL_WindowFlags(sdl_flags | SDL_WINDOW_METAL);
                #else
                throw std::invalid_argument("Metal contexts require SDL 2.0.14 or newer");
                #endif
                #else
                throw std::invalid_argument("Metal contexts are not supported on non-Apple platforms");
                #endif
            } else if ((g_window_flags & WindowCreationFlags::DirectX) != 0) {
                Logger::default_logger().fatal("DirectX contexts are not supported at this time");
            } else if ((g_window_flags & WindowCreationFlags::WebGPU) != 0) {
                Logger::default_logger().fatal("WebGPU contexts are not supported at this time");
            }

            //sdl_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;
            pimpl->handle = SDL_CreateWindow(get_client_name().c_str(),
                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                    DEF_WINDOW_DIM, DEF_WINDOW_DIM,
                    sdl_flags);
            if (pimpl->handle == nullptr) {
                Logger::default_logger().fatal("Failed to create SDL window");
            }


            {
                std::unique_lock<std::shared_mutex> lock(g_window_maps_mutex);
                g_window_handle_map.insert({ pimpl->handle, this });
            }

            _register_callbacks(pimpl->handle);

            pimpl->state |= WINDOW_STATE_CREATED;

            //TODO: figure out how to handle content scale
            pimpl->content_scale = { 1.0, 1.0 };

            dispatch_event<WindowEvent>(WindowEventType::Create, *this);

            return;
        }

        if (!(pimpl->state & WINDOW_STATE_COMMITTED)) {
            return;
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
            SDL_SetWindowTitle(pimpl->handle, title->c_str());
        }

        if (fullscreen.dirty
                || (fullscreen && (display.dirty || custom_display_mode.dirty || display_mode.dirty))) {
            if (fullscreen) {
                // switch to fullscreen mode or display/display mode

                const Display &target_display = get_display_affinity();

                auto disp_off = target_display.get_position();
                SDL_SetWindowPosition(pimpl->handle, disp_off.x, disp_off.y);
                SDL_SetWindowFullscreen(pimpl->handle, SDL_WINDOW_FULLSCREEN);
                SDL_DisplayMode sdl_mode;
                if (custom_display_mode) {
                    SDL_DisplayMode cur_mode = unwrap_display_mode(display_mode);
                    SDL_GetClosestDisplayMode(target_display.pimpl->index, &cur_mode, &sdl_mode);
                    assert(sdl_mode.w > 0);
                    assert(sdl_mode.h > 0);
                    SDL_SetWindowDisplayMode(pimpl->handle, &sdl_mode);
                } else {
                    SDL_GetDesktopDisplayMode(target_display.pimpl->index, &sdl_mode);
                    SDL_SetWindowDisplayMode(pimpl->handle, &sdl_mode);
                }

                affirm_precond(sdl_mode.refresh_rate <= UINT16_MAX, "Refresh rate is too big");
                pimpl->cur_resolution = { uint32_t(sdl_mode.w), uint32_t(sdl_mode.h) };
                pimpl->cur_refresh_rate = uint16_t(sdl_mode.refresh_rate);
            } else {
                affirm_precond(windowed_res->x <= INT_MAX && windowed_res->y <= INT_MAX,
                        "Current windowed resolution is too large");

                SDL_SetWindowFullscreen(pimpl->handle, 0);

                auto &target_disp = get_display_affinity();
                int pos_x = target_disp.get_position().x + position->x;
                int pos_y = target_disp.get_position().y + position->y;
                SDL_SetWindowPosition(pimpl->handle, pos_x, pos_y);

                SDL_SetWindowSize(pimpl->handle, int(windowed_res->x), int(windowed_res->y));

                pimpl->cur_resolution = windowed_res;
            }
        } else if (!fullscreen) {
            // update windowed positon and/or resolution

            if (windowed_res.dirty) {
                affirm_precond(windowed_res->x <= INT_MAX && windowed_res->y <= INT_MAX,
                        "Current windowed resolution is too large");

                SDL_SetWindowSize(pimpl->handle, int(windowed_res->x), int(windowed_res->y));

                pimpl->cur_resolution = windowed_res;
            }

            if (position.dirty) {
                auto &target_disp = get_display_affinity();
                int pos_x = target_disp.get_position().x + position->x;
                int pos_y = target_disp.get_position().y + position->y;
                SDL_SetWindowPosition(pimpl->handle, pos_x, pos_y);
            }
        }

        if (mouse_capture.dirty || mouse_visible.dirty) {
            if (mouse_capture && !mouse_visible) {
                if (mouse_raw_input.dirty) {
                    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, mouse_raw_input ? "0" : "1");
                }
                SDL_SetRelativeMouseMode(SDL_TRUE);
            } else {
                #if SDL_VERSION_ATLEAST(2, 0, 16)
                SDL_SetWindowMouseGrab(pimpl->handle, mouse_capture ? SDL_TRUE : SDL_FALSE);
                #else
                SDL_SetWindowGrab(pimpl->handle, mouse_capture ? SDL_TRUE : SDL_FALSE);
                #endif
                //TODO: not great, would be better to set it per window somehow
                SDL_ShowCursor(mouse_visible ? SDL_TRUE : SDL_FALSE);
            }
        }

        if (!(pimpl->state & WINDOW_STATE_READY)) {
            pimpl->state |= WINDOW_STATE_READY;
        }

        if (!(pimpl->state & WINDOW_STATE_VISIBLE)) {
            SDL_ShowWindow(pimpl->handle);
            pimpl->state |= WINDOW_STATE_VISIBLE;
        }

        _dispatch_window_update_event(*this, delta);
        UNUSED(delta);

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
            auto *found = get_display_from_index(display->pimpl->index);
            if (found != nullptr) {
                return *found;
            }
        }

        auto *primary = get_display_from_index(0);

        if (primary == nullptr) {
            throw std::runtime_error("No available displays!");
        }

        return *primary;
    }

    void Window::set_display_affinity(const Display &display) {
        auto *cur_display = pimpl->properties.display.peek();
        if (cur_display != nullptr && display.pimpl->index == cur_display->pimpl->index) {
            return;
        }

        auto *found = get_display_from_index(display.pimpl->index);
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
            SDL_DisplayMode desktop_mode;
            SDL_GetDesktopDisplayMode(get_display_affinity().pimpl->index, &desktop_mode);
            return wrap_display_mode(desktop_mode);
        }
    }

    void Window::set_display_mode(DisplayMode mode) {
        pimpl->properties.custom_display_mode = true;
        pimpl->properties.display_mode = mode;
    }


    bool Window::is_mouse_captured(void) const {
        return pimpl->properties.mouse_capture.peek();
    }

    void Window::set_mouse_captured(bool captured) {
        pimpl->properties.mouse_capture = captured;
    }

    bool Window::is_mouse_visible(void) const {
        return pimpl->properties.mouse_visible.peek();
    }

    void Window::set_mouse_visible(bool visible) {
        pimpl->properties.mouse_visible = visible;
    }

    bool Window::is_mouse_raw_input(void) const {
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
        std::shared_lock<std::shared_mutex> lock(g_window_maps_mutex);

        auto it = g_window_handle_map.find(static_cast<SDL_Window *>(const_cast<void *>(handle)));
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
            // if not in fullscreen mode
            if ((SDL_GetWindowFlags(window.pimpl->handle) & SDL_WINDOW_FULLSCREEN) == 0) {
                window.pimpl->properties.position.set_quietly(event.position);
            }
        }
    }
}
