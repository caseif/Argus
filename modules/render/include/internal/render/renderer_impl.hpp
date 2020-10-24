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

namespace argus {
    // forward declarations
    class Material;
    class Renderer;
    class Shader;
    class TextureData;

    class RendererImpl {
       public:
        RendererImpl() {
        }

        virtual void init_context_hints(void) = 0;

        virtual void init(Renderer &renderer) = 0;

        virtual void deinit_texture(const TextureData &texture) = 0;

        virtual void deinit_shader(const Shader &shader) = 0;
        
        virtual void deinit_material(const Material &material) = 0;

        virtual void render(Renderer &renderer, const TimeDelta delta) = 0;
    };
}
