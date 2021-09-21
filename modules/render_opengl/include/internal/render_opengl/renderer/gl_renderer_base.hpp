/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
