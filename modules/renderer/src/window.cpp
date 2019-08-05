// module core
#include "argus/core.hpp"
#include "internal/util.hpp"

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

namespace argus {

    extern bool g_renderer_initialized;

    extern std::vector<Window*> g_windows;
    extern size_t g_window_count;

    void _window_event_callback(void *data, SDL_Event &event) {
        Window *window = static_cast<Window*>(data);
        if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
            window->destroy();
        }
    }

    Window::Window(void) {
        _ARGUS_ASSERT(g_renderer_initialized, "Cannot create window before renderer module is initialized.");

        handle = SDL_CreateWindow("ArgusGame",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                DEF_WINDOW_DIM, DEF_WINDOW_DIM,
                SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

        g_window_count++;
        g_windows.insert(g_windows.cend(), this);

        parent = nullptr;

        renderer = new Renderer(*this);
        
        // register the listener
        listener_id = register_sdl_event_listener(event_filter, _window_event_callback, this);

        callback_id = register_render_callback(std::bind(&Window::update, this, std::placeholders::_1));

        return;
    }

    Window::~Window(void) = default;

    void Window::destroy(void) {
        unregister_render_callback(callback_id);
        unregister_sdl_event_listener(listener_id);

        for (Window *child : children) {
            child->parent = nullptr;
            child->destroy();
        }

        if (parent != nullptr) {
            parent->remove_child(*this);
        }

        renderer->destroy();

        SDL_DestroyWindow(handle);

        remove_from_vector(g_windows, this);

        delete this;

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

    Renderer &Window::get_renderer(void) const {
        return *renderer;
    }

    void Window::update(const Timestamp delta) {
        SDL_UpdateWindowSurface(handle);
        return;
    }

    void Window::set_title(const std::string &title) {
        SDL_SetWindowTitle(handle, title.c_str());
        return;
    }

    void Window::set_fullscreen(const bool fullscreen) {
        SDL_SetWindowFullscreen(handle, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        return;
    }

    void Window::set_resolution(const unsigned int width, const unsigned int height) {
        SDL_SetWindowSize(handle, width, height);
        return;
    }

    void Window::set_windowed_position(const unsigned int x, const unsigned int y) {
        SDL_SetWindowPosition(handle, x, y);
        return;
    }

    void Window::activate(void) {
        SDL_ShowWindow(handle);
        return;
    }

    int Window::event_filter(void *data, SDL_Event *event) {
        Window *window = static_cast<Window*>(data);
        return event->type == SDL_WINDOWEVENT && event->window.windowID == SDL_GetWindowID(window->handle);
    }

}
