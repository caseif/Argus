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

#pragma once

#include "internal/render_opengl/types.hpp"

#include "aglet/aglet.h"

#include <cstddef>

namespace argus {
    struct BufferInfo {
        bool valid;
        size_t size;
        GLenum target;
        buffer_handle_t handle;
        void *mapped;
        bool allow_mapping;
        bool persistent;

        static BufferInfo create(GLenum target, size_t size, GLenum usage, bool allow_mapping, bool map_nonpersistent);

        void destroy(void);

        void map_write();

        void unmap();

        void write(void *src, size_t size, size_t offset);

        template <typename T, std::enable_if_t<!std::is_pointer_v<T>, bool> = true>
        void write_val(T val, size_t offset) {
            write(&val, sizeof(T), offset);
        }

        void clear(uint32_t value);
    };
}
