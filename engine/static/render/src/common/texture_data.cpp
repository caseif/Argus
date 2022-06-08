/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/memory.hpp"

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

    const unsigned char *const *&TextureData::get_pixel_data(void) const {
        return const_cast<const unsigned char *const *&>(pimpl->image_data);
    }
}
