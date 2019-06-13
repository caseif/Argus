#include "argus/renderer.hpp"

#include <iostream>
#include <string>
#include <SDL2/SDL_render.h>

#define DEF_TITLE "ArgusGame"
#define DEF_WINDOW_DIM 300

namespace argus {

    Window::Window(void) {
        handle = SDL_CreateWindow("ArgusGame",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                DEF_WINDOW_DIM, DEF_WINDOW_DIM,
                SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
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

    void Window::set_windowed_resolution(unsigned int width, unsigned int height) {
        SDL_SetWindowSize(handle, width, height);
        return;
    }

    void Window::set_windowed_position(unsigned int x, unsigned int y) {
        SDL_SetWindowPosition(handle, x, y);
        return;
    }

    void Window::activate(void) {
        //TODO
        return;
    }

    SDL_Window *Window::get_sdl_window(void) {
        return handle;
    }

}
