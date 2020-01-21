/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/math.hpp"
#include "argus/threading.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core/core_util.hpp"

// module renderer
#include "argus/renderer/renderer.hpp"
#include "argus/renderer/window.hpp"
#include "argus/renderer/window_event.hpp"
#include "internal/renderer/pimpl/renderer.hpp"
#include "internal/renderer/pimpl/window.hpp"

#include <SDL2/SDL_video.h>

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <cstddef>
#include <cstdint>

#define DEF_TITLE "ArgusGame"
#define DEF_WINDOW_DIM 300

#define WINDOW_STATE_INITIALIZED        1
#define WINDOW_STATE_READY              2
#define WINDOW_STATE_VISIBLE            4
#define WINDOW_STATE_CLOSE_REQUESTED    8
#define WINDOW_STATE_VALID              16

namespace argus {

    extern bool g_renderer_initialized;

    extern std::map<uint32_t, Window*> g_window_map;
    extern size_t g_window_count;

    Window::Window(void): pimpl(new pimpl_Window(*this)) {
        _ARGUS_ASSERT(g_renderer_initialized, "Cannot create window before renderer module is initialized.");

        pimpl->handle = SDL_CreateWindow("ArgusGame",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            DEF_WINDOW_DIM, DEF_WINDOW_DIM,
            SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        pimpl->state = WINDOW_STATE_VALID;
        pimpl->close_callback = nullptr;

        g_window_count++;
        g_window_map.insert({SDL_GetWindowID(static_cast<SDL_Window*>(pimpl->handle)), this});

        pimpl->parent = nullptr;

        // register the listener
        pimpl->listener_id = register_event_handler(event_filter, event_callback, this);

        pimpl->callback_id = register_render_callback(std::bind(&Window::update, this, std::placeholders::_1));

        return;
    }

    Window::~Window(void) {
        delete pimpl;
    }

    void Window::destroy(void) {
        pimpl->state &= ~WINDOW_STATE_VALID;

        pimpl->renderer.destroy();

        if (pimpl->close_callback) {
            pimpl->close_callback(*this);
        }

        unregister_render_callback(pimpl->callback_id);
        unregister_event_handler(pimpl->listener_id);

        for (Window *child : pimpl->children) {
            child->pimpl->parent = nullptr;
            child->pimpl->state |= WINDOW_STATE_CLOSE_REQUESTED;
        }

        if (pimpl->parent != nullptr) {
            pimpl->parent->remove_child(*this);
        }

        g_window_map.erase(SDL_GetWindowID(static_cast<SDL_Window*>(pimpl->handle)));

        SDL_DestroyWindow(static_cast<SDL_Window*>(pimpl->handle));

        if (--g_window_count == 0) {
            stop_engine();
        }

        return;
    }

    Window &Window::create_window(void) {
        return *new Window();
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

    Renderer &Window::get_renderer(void) {
        return pimpl->renderer;
    }

    void Window::update(const Timestamp delta) {
        if (!(pimpl->state & WINDOW_STATE_VALID)) {
            delete this;
            return;
        }

        if (!(pimpl->state & WINDOW_STATE_INITIALIZED)) {
            pimpl->renderer.init();
            pimpl->state |= WINDOW_STATE_INITIALIZED;
            return;
        }

        SDL_Window *sdl_handle = static_cast<SDL_Window*>(pimpl->handle);

        if (!(pimpl->state & WINDOW_STATE_VISIBLE) && (pimpl->state & WINDOW_STATE_READY)) {
            SDL_ShowWindow(sdl_handle);
        }

        if (pimpl->state & WINDOW_STATE_CLOSE_REQUESTED) {
            destroy();
            return;
        }

        if (pimpl->properties.title.dirty) {
            SDL_SetWindowTitle(sdl_handle, ((std::string) pimpl->properties.title).c_str());
        }
        if (pimpl->properties.fullscreen.dirty) {
            SDL_SetWindowFullscreen(sdl_handle, pimpl->properties.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        }
        if (pimpl->properties.resolution.dirty) {
            Vector2u res = pimpl->properties.resolution;
            SDL_SetWindowSize(sdl_handle, res.x, res.y);
            pimpl->renderer.pimpl->dirty_resolution = true;
        }
        if (pimpl->properties.position.dirty) {
            Vector2i pos = pimpl->properties.position;
            SDL_SetWindowPosition(sdl_handle, pos.x, pos.y);
        }

        pimpl->renderer.render(delta);

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

    void Window::set_fullscreen(const bool fullscreen) {
        pimpl->properties.fullscreen = fullscreen;
        return;
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
        pimpl->state |= WINDOW_STATE_READY;
        return;
    }

    const bool Window::event_filter(const ArgusEvent &event, void *user_data) {
        const WindowEvent &window_event = static_cast<const WindowEvent&>(event);
        Window *window = static_cast<Window*>(user_data);

        // ignore events for uninitialized windows
        if (!(window->pimpl->state & WINDOW_STATE_INITIALIZED)) {
            return false;
        }

        return event.type == ArgusEventType::WINDOW && window_event.window == window;
    }

    void Window::event_callback(const ArgusEvent &event, void *user_data) {
        const WindowEvent &window_event = static_cast<const WindowEvent&>(event);
        Window *window = static_cast<Window*>(user_data);

        if (window_event.subtype == WindowEventType::CLOSE) {
            window->pimpl->state |= WINDOW_STATE_CLOSE_REQUESTED;
        }
    }

}
