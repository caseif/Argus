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

#include "argus/render/cabi/common/texture_data.h"

#include "argus/render/common/texture_data.hpp"

static inline const argus::TextureData &_as_ref(argus_texture_data_const_t tex_data) {
    return *reinterpret_cast<const argus::TextureData *>(tex_data);
}

extern "C" {

uint32_t argus_texture_data_get_width(argus_texture_data_const_t tex_data) {
    return _as_ref(tex_data).m_width;
}

uint32_t argus_texture_data_get_height(argus_texture_data_const_t tex_data) {
    return _as_ref(tex_data).m_height;
}

const unsigned char *const *argus_texture_data_get_pixel_data(argus_texture_data_const_t tex_data) {
    return _as_ref(tex_data).get_pixel_data();
}

}
