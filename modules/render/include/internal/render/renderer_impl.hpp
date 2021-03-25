/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/time.hpp"

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

        virtual void init(Renderer &renderer) = 0;

        virtual void deinit(Renderer &renderer) = 0;

        virtual void deinit_texture(const TextureData &texture) = 0;

        virtual void deinit_shader(const Shader &shader) = 0;
        
        virtual void deinit_material(const Material &material) = 0;

        virtual void render(Renderer &renderer, const TimeDelta delta) = 0;
    };
}
