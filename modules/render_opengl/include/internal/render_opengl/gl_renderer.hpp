/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "internal/render/renderer_impl.hpp"

namespace argus {
    class GLRenderer : public RendererImpl {
       public:
        GLRenderer(Renderer &renderer);

        void init_context_hints(void) override;

        void init(void) override;

        void render(const TimeDelta delta) override;
    };
}
