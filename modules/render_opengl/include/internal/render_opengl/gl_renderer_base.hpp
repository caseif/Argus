/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "internal/render/renderer_impl.hpp"

#include <map>

namespace argus {
    // forward declarations
    class Renderer;

    struct RendererState;

    class GLRenderer : public RendererImpl {
        private:
            std::map<const Renderer*, RendererState> renderer_states;

        public:
            GLRenderer(void);

            void init(Renderer &renderer) override;

            void deinit(Renderer &renderer) override;

            void deinit_texture(const TextureData &texture) override;

            void deinit_shader(const Shader &shader) override;

            void deinit_material(const Material &material) override;

            void render(Renderer &renderer, const TimeDelta delta) override;

            RendererState &get_renderer_state(Renderer &renderer);
    };
}
