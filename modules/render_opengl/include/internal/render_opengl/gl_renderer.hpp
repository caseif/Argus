/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "internal/render/renderer_impl.hpp"

namespace argus {
    // forward declarations
    class Renderer;

    class GLRenderer : public RendererImpl {
       public:
        GLRenderer(void);

        void init_context_hints(void) override;

        void init(Renderer &renderer) override;

        void deinit_texture(const TextureData &texture) override;
        
        void deinit_shader(const Shader &shader) override;
        
        void deinit_material(const Material &material) override;

        void render(Renderer &renderer, const TimeDelta delta) override;
    };
}
