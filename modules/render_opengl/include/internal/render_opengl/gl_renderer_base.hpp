/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module core
#include "argus/core/event.hpp"

// module render
#include "internal/render/renderer_impl.hpp"

#include <map>
#include <string>

namespace argus {
    // forward declarations
    class Renderer;

    struct RendererState;

    class GLRenderer : public RendererImpl {
        private:
            std::map<const Renderer*, RendererState> renderer_states;
            Index resource_event_handler;

        public:
            GLRenderer(void);

            void init(Renderer &renderer) override;

            void deinit(Renderer &renderer) override;

            void render(Renderer &renderer, const TimeDelta delta) override;

            RendererState &get_renderer_state(Renderer &renderer);
    };
}
