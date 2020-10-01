/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module render
#include "argus/render/renderer.hpp"
#include "argus/render/texture_data.hpp"
#include "internal/render/pimpl/texture_data.hpp"

#include "GLFW/glfw3.h"

#include <atomic>

#include <cstdio>

namespace argus {

    // IMPORTANT: image_data is assumed to be allocated on the heap
    TextureData::TextureData(const unsigned int width, const unsigned int height, unsigned char **&&image_data):
            pimpl(new pimpl_TextureData()),
            width(width),
            height(height) {
        image_data = image_data;
        image_data = nullptr;
    }

    TextureData::~TextureData(void) {
        for (size_t y = 0; y < height; y++) {
            delete[] pimpl->image_data[y];
        }
        delete[] pimpl->image_data;
    }

}
