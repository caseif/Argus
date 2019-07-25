#include "argus/core.hpp"
#include "argus/renderer.hpp"
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

namespace argus {

    extern bool g_renderer_initialized;

    std::vector<Window*> g_windows;
    size_t g_window_count = 0;

    int _window_event_filter(void *data, SDL_Event *event) {
        Window *window = static_cast<Window*>(data);
        return event->type == SDL_WINDOWEVENT && event->window.windowID == SDL_GetWindowID(window->get_sdl_window());
    }

    void _window_event_callback(void *data, SDL_Event *event) {
        Window *window = static_cast<Window*>(data);
        if (event->window.event == SDL_WINDOWEVENT_CLOSE) {
            window->destroy();
        }
    }

    Window::Window(void) {
        ASSERT(g_renderer_initialized, "Cannot create window before renderer module is initialized.");

        handle = SDL_CreateWindow("ArgusGame",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                DEF_WINDOW_DIM, DEF_WINDOW_DIM,
                SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

        g_window_count++;
        g_windows.insert(g_windows.cend(), this);
        
        // register the listener
        register_sdl_event_listener(_window_event_filter, _window_event_callback, this);

        register_render_callback(std::bind(&Window::update, this, std::placeholders::_1));

        return;
    }

    Window::~Window(void) = default;

    void Window::destroy(void) {
        for (Window *child : children) {
            child->parent = nullptr;
            child->destroy();
        }

        if (parent != nullptr) {
            parent->remove_child(this);
        }

        SDL_DestroyWindow(handle);

        g_windows.erase(std::remove(g_windows.begin(), g_windows.end(), this));

        delete this;

        if (--g_window_count == 0) {
            stop_engine();
        }

        return;
    }

    Window *Window::create_window(void) {
        return new Window();
    }

    Renderer *Window::create_renderer(void) {
        if (SDL_GetRenderer(handle) != NULL) {
            std::cerr << "create_renderer invoked on window twice" << std::endl;
        }

        return new Renderer(this);
    }

    Window *Window::create_child_window(void) {
        Window *child_window = new Window();
        child_window->parent = this;

        children.insert(children.cend(), child_window);

        return child_window;
    }

    void Window::remove_child(Window *child) {
        children.erase(std::remove(children.begin(), children.end(), child));
    }

    void Window::update(unsigned long long delta) {
        SDL_PumpEvents();
        SDL_UpdateWindowSurface(get_sdl_window());
        return;
    }

    void Window::set_title(std::string title) {
        SDL_SetWindowTitle(handle, title.c_str());
        return;
    }

    void Window::set_fullscreen(bool fullscreen) {
        SDL_SetWindowFullscreen(handle, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        return;
    }

    void Window::set_resolution(unsigned int width, unsigned int height) {
        SDL_SetWindowSize(handle, width, height);
        return;
    }

    void Window::set_windowed_position(unsigned int x, unsigned int y) {
        SDL_SetWindowPosition(handle, x, y);
        return;
    }

    void Window::activate(void) {
        SDL_ShowWindow(handle);
        return;
    }

    SDL_Window *Window::get_sdl_window(void) {
        return handle;
    }

}
