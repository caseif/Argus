/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/common/texture_data.hpp"
#include "internal/render/pimpl/common/texture_data.hpp"

#include <cstdio>

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_TextureData));

    // IMPORTANT: image_data is assumed to be allocated on the heap
    TextureData::TextureData(const unsigned int width, const unsigned int height, unsigned char **&&image_data):
            pimpl(&g_pimpl_pool.construct<pimpl_TextureData>(image_data)),
            width(width),
            height(height) {
    }

    TextureData::TextureData(const TextureData &rhs) noexcept:
        pimpl(&g_pimpl_pool.construct<pimpl_TextureData>(*rhs.pimpl)),
        width(rhs.width),
        height(rhs.height) {
    }

    TextureData::TextureData(TextureData &&rhs) noexcept:
        pimpl(rhs.pimpl),
        width(rhs.width),
        height(rhs.height) {
        rhs.pimpl = nullptr;
    }

    TextureData::~TextureData(void) {
        if (pimpl == nullptr) {
            return;
        }

        for (size_t y = 0; y < height; y++) {
            delete[] pimpl->image_data[y];
        }
        delete[] pimpl->image_data;

        g_pimpl_pool.destroy(pimpl);
    }
}
