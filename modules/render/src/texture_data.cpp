/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/memory.hpp"
#include "internal/lowlevel/logging.hpp"

// module render
#include "argus/render/texture_data.hpp"
#include "internal/render/glext.hpp"
#include "internal/render/pimpl/texture_data.hpp"

#include <GL/gl.h>

#include <atomic>

#include <cstdio>

namespace argus {

    using namespace glext;

    static AllocPool g_pimpl_pool(sizeof(pimpl_TextureData));

    // IMPORTANT: image_data is assumed to be allocated on the heap
    TextureData::TextureData(const size_t width, const size_t height, unsigned char **&&image_data):
            width(width),
            height(height),
            pimpl(&g_pimpl_pool.construct<pimpl_TextureData>()) {
        pimpl->image_data = image_data;
        pimpl->prepared = false;
        pimpl->image_data = nullptr;
    }

    TextureData::~TextureData(void) {
        if (pimpl->prepared) {
            glDeleteBuffers(1, &pimpl->buffer_handle);
        } else {
            for (size_t y = 0; y < height; y++) {
                delete[] pimpl->image_data[y];
            }
            delete[] pimpl->image_data;
        }

        g_pimpl_pool.free(pimpl);
    }

    const bool TextureData::is_prepared(void) {
        return pimpl->prepared;
    }

    void TextureData::prepare(void) {
        _ARGUS_ASSERT(!pimpl->prepared, "TextureData#prepare called twice\n");

        glGenBuffers(1, &pimpl->buffer_handle);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pimpl->buffer_handle);

        if (!glIsBuffer(pimpl->buffer_handle)) {
            _ARGUS_FATAL("Failed to gen pixel buffer during texture preparation\n");
        }

        size_t row_size = width * 32 / 8;
        glBufferData(GL_PIXEL_UNPACK_BUFFER, height * row_size, nullptr, GL_STREAM_COPY);

        size_t offset = 0;
        for (size_t y = 0; y < height; y++) {
            glBufferSubData(GL_PIXEL_UNPACK_BUFFER, offset, row_size, pimpl->image_data[y]);
            offset += row_size;
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        for (size_t y = 0; y < height; y++) {
            delete[] pimpl->image_data[y];
        }
        delete[] pimpl->image_data;

        pimpl->image_data = nullptr;

        pimpl->prepared = true;
    }

}
