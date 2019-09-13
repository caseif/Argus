#pragma once

#include "argus/core.hpp"

#include <SDL2/SDL_events.h>

namespace argus {

    typedef std::function<bool(SDL_Event&, void*)> SDLEventFilter;

    typedef std::function<void(SDL_Event&, void*)> SDLEventCallback;

    const Index register_sdl_event_handler(const SDLEventFilter filter, const SDLEventCallback callback,
            void *const data);

    void unregister_sdl_event_handler(const Index id);

}
