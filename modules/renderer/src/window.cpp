// module renderer
#include "argus/renderer.hpp"

// module pi
#include "argus/threading.hpp"

// module core
#include "argus/core.hpp"
#include "internal/util.hpp"

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
            state(WINDOW_STATE_VALID) {
        _ARGUS_ASSERT(g_renderer_initialized, "Cannot create window before renderer module is initialized.");

        g_window_count++;
        g_windows.insert(g_windows.cend(), this);

        parent = nullptr;
        
        // register the listener
        listener_id = register_sdl_event_listener(event_filter, window_event_callback, this);

        callback_id = register_render_callback(std::bind(&Window::update, this, std::placeholders::_1));

        return;
    }

    Window::~Window(void) = default;

    void Window::init(void) {
        handle = SDL_CreateWindow("ArgusGame",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                DEF_WINDOW_DIM, DEF_WINDOW_DIM,
                SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        SDL_GetWindowSurface(handle);

        state |= WINDOW_STATE_INITIALIZED;
    }

    void Window::destroy(void) {
        if (close_callback != nullptr) {
            close_callback(*this);
        }

        unregister_render_callback(callback_id);
        unregister_sdl_event_listener(listener_id);

        for (Window *child : children) {
            child->parent = nullptr;
            child->state |= WINDOW_STATE_CLOSE_REQUESTED;
        }

        if (parent != nullptr) {
            parent->remove_child(*this);
        }

        renderer.destroy();

        SDL_DestroyWindow(handle);

        remove_from_vector(g_windows, this);

        if (--g_window_count == 0) {
            stop_engine();
        }

        state &= ~WINDOW_STATE_VALID;

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

        if (!(state & WINDOW_STATE_INITIALIZED)) {
            init();
        }

        if (!(state & WINDOW_STATE_VISIBLE) && (state & WINDOW_STATE_READY)) {
            SDL_ShowWindow(handle);
        }

        if (state & WINDOW_STATE_CLOSE_REQUESTED) {
            destroy();
            return;
        }

        if (!renderer.initialized) {
            renderer.init();
        }

        if (properties.title.dirty) {
            SDL_SetWindowTitle(handle, ((std::string) properties.title).c_str());
        }
        if (properties.fullscreen.dirty) {
            SDL_SetWindowFullscreen(handle, properties.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        }
        if (properties.resolution.dirty) {
            pair_t<unsigned int> res = properties.resolution;
            SDL_SetWindowSize(handle, res.a, res.b);
        }
        if (properties.position.dirty) {
            pair_t<int> pos = properties.position;
            SDL_SetWindowPosition(handle, pos.a, pos.b);
        }

        renderer.render(delta);

        SDL_GL_SwapWindow(handle);
        SDL_UpdateWindowSurface(handle);
        return;
    }

    void Window::set_title(std::string const &title) {
        properties.title = title;
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
