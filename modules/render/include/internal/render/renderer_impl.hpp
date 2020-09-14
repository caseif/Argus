/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */
#pragma once

// module core
#include "argus/core.hpp"

// module render
#include "argus/render/renderer.hpp"

namespace argus {
    class RendererImpl {
       protected:
        Renderer &renderer;

       public:
        RendererImpl(Renderer &renderer): renderer(renderer) {
        }

        virtual void init_context_hints(void) = 0;

        virtual void init(void) = 0;

        virtual void render(const TimeDelta delta) = 0;
    };
}
