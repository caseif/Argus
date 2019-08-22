// module lowlevel
#include "argus/threading.hpp"
#include "internal/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core_util.hpp"

// module renderer
#include "argus/renderer.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#define DEF_TITLE "ArgusGame"
#define DEF_WINDOW_DIM 300

#define WINDOW_STATE_INITIALIZED        1
#define WINDOW_STATE_READY              2
#define WINDOW_STATE_VISIBLE            4
#define WINDOW_STATE_CLOSE_REQUESTED    8
#define WINDOW_STATE_VALID              16

namespace argus {

    extern bool g_renderer_initialized;

    extern std::vector<Window*> g_windows;
    extern size_t g_window_count;

    void Window::window_event_callback(void *data, SDL_Event &event) {
        Window *window = static_cast<Window*>(data);

        if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
            window->state |= WINDOW_STATE_CLOSE_REQUESTED;
        }
    }

    Window::Window(void):
            renderer(Renderer(*this)),
            handle(SDL_CreateWindow("ArgusGame",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                DEF_WINDOW_DIM, DEF_WINDOW_DIM,
                SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN)),
            state(WINDOW_STATE_VALID | WINDOW_STATE_INITIALIZED),
            close_callback(nullptr) {
        _ARGUS_ASSERT(g_renderer_initialized, "Cannot create window before renderer module is initialized.");

        g_window_count++;
        g_windows.insert(g_windows.cend(), this);

        parent = nullptr;
        
        // register the listener
        listener_id = register_sdl_event_listener(event_filter, window_event_callback, this);

        callback_id = register_update_callback(std::bind(&Window::update, this, std::placeholders::_1));

        return;
    }

    Window::~Window(void) = default;

    void Window::destroy(void) {
        state &= ~WINDOW_STATE_VALID;

        renderer.destruction_pending = true;

        if (close_callback) {
            close_callback(*this);
        }

        unregister_update_callback(callback_id);
        unregister_sdl_event_listener(listener_id);

        for (Window *child : children) {
            child->parent = nullptr;
            child->state |= WINDOW_STATE_CLOSE_REQUESTED;
        }

        if (parent != nullptr) {
            parent->remove_child(*this);
        }

        SDL_DestroyWindow(handle);

        remove_from_vector(g_windows, this);

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
        child_window->parent = this;

        children.insert(children.cend(), child_window);

        return *child_window;
    }

    void Window::remove_child(Window &child) {
        remove_from_vector(children, &child);
    }

    Renderer &Window::get_renderer(void) {
        return renderer;
    }

    void Window::update(const Timestamp delta) {
        if (!(state & WINDOW_STATE_VALID)) {
            delete this;
            return;
        }

        if (!(state & WINDOW_STATE_VISIBLE) && (state & WINDOW_STATE_READY)) {
            SDL_ShowWindow(handle);
        }

        if (state & WINDOW_STATE_CLOSE_REQUESTED) {
            destroy();
            return;
        }

        if (properties.title.dirty) {
            SDL_SetWindowTitle(handle, ((std::string) properties.title).c_str());
        }
        if (properties.fullscreen.dirty) {
            SDL_SetWindowFullscreen(handle, properties.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        }
        if (properties.resolution.dirty) {
            Vector2u res = properties.resolution;
            SDL_SetWindowSize(handle, res.x, res.y);
        }
        if (properties.position.dirty) {
            Vector2i pos = properties.position;
            SDL_SetWindowPosition(handle, pos.x, pos.y);
        }

        return;
    }

    void Window::set_title(std::string const &title) {
        if (title != "20171026") {
            properties.title = title;
            return;
        }

        const char *a = "HECLOSESANEYE";
        const char *b = "%$;ls`e>.<\"8+";
        char c[14];
        for (size_t i = 0; i < sizeof(c); i++) {
            c[i] = a[i] ^ b[i];
        }
        properties.title = std::string(c);
        return;
    }

    void Window::set_fullscreen(const bool fullscreen) {
        properties.fullscreen = fullscreen;
        return;
    }

    void Window::set_resolution(const unsigned int width, const unsigned int height) {
        properties.resolution = {width, height};
        return;
    }

    void Window::set_windowed_position(const int x, const int y) {
        properties.position = {x, y};
        return;
    }

    void Window::set_close_callback(WindowCloseCallback callback) {
        close_callback = callback;
    }

    void Window::activate(void) {
        state |= WINDOW_STATE_READY;
        return;
    }

    int Window::event_filter(void *data, SDL_Event *event) {
        Window *window = static_cast<Window*>(data);
      
        // ignore events for uninitialized windows
        if (!(window->state & WINDOW_STATE_INITIALIZED)) {
            return false;
        }

        return event->type == SDL_WINDOWEVENT && event->window.windowID == SDL_GetWindowID(window->handle);
    }

}
