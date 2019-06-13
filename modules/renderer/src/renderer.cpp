#include "argus/renderer.hpp"

#include <iostream>
#include <SDL2/SDL_render.h>

namespace argus {

    Renderer::Renderer(Window *window) {
        handle = SDL_CreateRenderer(window->get_sdl_window(), -1, SDL_RENDERER_ACCELERATED);
    }

    SDL_Renderer *Renderer::get_sdl_renderer(void) {
        return handle;
    }

}
