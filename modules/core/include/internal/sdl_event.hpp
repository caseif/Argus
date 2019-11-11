/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

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