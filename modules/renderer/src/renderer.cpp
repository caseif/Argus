#include "argus/renderer.hpp"

#include <SDL2/SDL_render.h>

namespace argus {

    Renderer::Renderer(Window *window) {
        //TODO
    }

    SDL_Renderer *Renderer::get_sdl_renderer(void) {
        return handle;
    }

}
