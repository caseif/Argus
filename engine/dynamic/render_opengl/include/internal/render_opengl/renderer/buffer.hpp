/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#pragma once

#include "aglet/aglet.h"

#include <cstddef>

namespace argus {
    struct BufferInfo {
        size_t len;
        buffer_handle_t buffer;
        void *mapped;
        bool persistent;
    };

    BufferInfo create_buffer(GLenum target, size_t len, GLenum usage, bool map_nonpersistent);

    void map_buffer_w(BufferInfo &buffer, GLenum target);

    void unmap_buffer(BufferInfo &buffer, GLenum target);

    void write_buffer_data(BufferInfo &buffer, size_t offset, size_t size, void *src);

    template <typename T, std::enable_if_t<!std::is_pointer_v<T>, bool> = true>
    void write_buffer_val(BufferInfo &buffer, size_t offset, T val) {
        write_buffer_data(buffer, offset, sizeof(T), &val);
    }
}