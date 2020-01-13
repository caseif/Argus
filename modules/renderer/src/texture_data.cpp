/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module renderer
#include "argus/renderer/texture_data.hpp"
#include "internal/renderer/glext.hpp"

#include <SDL2/SDL_opengl.h>

#include <atomic>

#include <cstdio>

namespace argus {

    using namespace glext;

    // IMPORTANT: image_data is assumed to be allocated on the heap
    TextureData::TextureData(const size_t width, const size_t height, unsigned char **&&image_data):
            width(width),
            height(height),
            image_data(image_data),
            prepared(false) {
        image_data = nullptr;
    }

    TextureData::~TextureData(void) {
        if (prepared) {
            glDeleteBuffers(1, &buffer_handle);
        } else {
            for (size_t y = 0; y < height; y++) {
                delete[] image_data[y];
            }
            delete[] image_data;
        }
    }

    const bool TextureData::is_prepared(void) {
        return prepared;
    }

    void TextureData::prepare(void) {
        _ARGUS_ASSERT(!prepared, "TextureData#prepare called twice\n");

        glGenBuffers(1, &buffer_handle);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer_handle);

        if (!glIsBuffer(buffer_handle)) {
            _ARGUS_FATAL("Failed to gen pixel buffer during texture preparation\n");
        }

        size_t row_size = width * 32 / 8;
        glBufferData(GL_PIXEL_UNPACK_BUFFER, height * row_size, nullptr, GL_STREAM_COPY);

        size_t offset = 0;
        for (size_t y = 0; y < height; y++) {
            glBufferSubData(GL_PIXEL_UNPACK_BUFFER, offset, row_size, image_data[y]);
            offset += row_size;
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        for (size_t y = 0; y < height; y++) {
            delete[] image_data[y];
        }
        delete[] image_data;

        image_data = nullptr;

        prepared = true;
    }

}
