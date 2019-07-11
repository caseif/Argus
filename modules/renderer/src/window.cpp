#include "argus/core.hpp"
#include "argus/lowlevel.hpp"
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

    typedef std::function<void(void*)> WindowEventCallback;

    extern bool g_renderer_initialized;

    std::vector<Window*> g_windows;

    void _event_callback(Window *window, SDL_Event *event) {
        switch (event->type) {
            case SDL_WINDOWEVENT:
                if (event->window.event == SDL_WINDOWEVENT_CLOSE) {
                    stop_engine();
                }
                break;
            default:
                break;
        }
    }

    Window::Window(void) {
        ASSERT(g_renderer_initialized, "Cannot create window before renderer module is initialized.");

        handle = SDL_CreateWindow("ArgusGame",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                DEF_WINDOW_DIM, DEF_WINDOW_DIM,
                SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        
        // add the close listener
        SDL_AddEventWatch(
                [](auto userdata, auto event) -> int{
                    _event_callback(static_cast<Window*>(userdata), event);
                    return 0;
                },
                this
        );

        register_render_callback(std::bind(&Window::update, this, std::placeholders::_1));

        return;
    }

    Window::~Window(void) = default;

    void Window::destroy(void) {
        g_windows.erase(std::remove(g_windows.begin(), g_windows.end(), this));
        delete this;
        return;
    }

    void Window::update(unsigned long long delta) {
        printf("update\n");
        SDL_PumpEvents();
        SDL_UpdateWindowSurface(get_sdl_window());
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
