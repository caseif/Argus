/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render_opengl
#include "internal/render_opengl/glfw_include.hpp"

#include <map>

namespace argus {
    // forward declarations
    class Renderer;

    typedef GLuint texture_handle_t;

    struct BufferedTexture {
        unsigned int width;
        unsigned int height;
        texture_handle_t gl_handle;

        BufferedTexture(unsigned int width, unsigned int height):
            width(width),
            height(height) {
        }
    };

    struct TextureList {
        std::map<Renderer*, BufferedTexture> textures;
    };
}
