/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_TextureData));

    // IMPORTANT: image_data is assumed to be allocated on the heap
    TextureData::TextureData(const unsigned int width, const unsigned int height, unsigned char **&&image_data) :
            m_pimpl(&g_pimpl_pool.construct<pimpl_TextureData>(image_data)),
            m_width(width),
            m_height(height) {
    }

    TextureData::TextureData(const TextureData &rhs) noexcept:
            m_pimpl(&g_pimpl_pool.construct<pimpl_TextureData>(*rhs.m_pimpl)),
            m_width(rhs.m_width),
            m_height(rhs.m_height) {
    }

    TextureData::TextureData(TextureData &&rhs) noexcept:
            m_pimpl(rhs.m_pimpl),
            m_width(rhs.m_width),
            m_height(rhs.m_height) {
        rhs.m_pimpl = nullptr;
    }

    TextureData::~TextureData(void) {
        if (m_pimpl == nullptr) {
            return;
        }

        for (size_t y = 0; y < m_height; y++) {
            delete[] m_pimpl->image_data[y];
        }
        delete[] m_pimpl->image_data;

        g_pimpl_pool.destroy(m_pimpl);
    }

    const unsigned char *const *&TextureData::get_pixel_data(void) const {
        return const_cast<const unsigned char *const *&>(m_pimpl->image_data);
    }
}
