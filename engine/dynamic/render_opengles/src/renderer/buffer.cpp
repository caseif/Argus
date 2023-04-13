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

#include "internal/render_opengles/types.hpp"
#include "internal/render_opengles/renderer/buffer.hpp"

#include "aglet/aglet.h"

#include <cassert>
#include <cstddef>
#include <cstring>

namespace argus {
    BufferInfo BufferInfo::create(GLenum target, size_t size, GLenum usage, bool map_nonpersistent) {
        buffer_handle_t handle;
        void *mapped = nullptr;
        bool persistent = false;

        glGenBuffers(1, &handle);
        glBindBuffer(target, handle);
        glBufferData(target, GLsizeiptr(size), nullptr, usage);
        if (map_nonpersistent) {
            mapped = glMapBufferRange(target, 0, GLsizeiptr(size), GL_MAP_WRITE_BIT);
        }

        return BufferInfo { true, size, target, handle, mapped, persistent };
    }

    void BufferInfo::destroy() {
        if (!valid) {
            return;
        }

        unmap();
        glDeleteBuffers(0, &handle);

        handle = 0;
        valid = false;
    }

    void BufferInfo::map_write() {
        assert(valid);

        if (persistent) {
            return;
        }

        assert(mapped == nullptr);

        glBindBuffer(target, handle);
        mapped = glMapBufferRange(target, 0, GLsizeiptr(size), GL_MAP_WRITE_BIT);
        glBindBuffer(target, 0);
    }

    void BufferInfo::unmap() {
        assert(valid);

        if (persistent) {
            return;
        }

        assert(mapped != nullptr);

        glBindBuffer(target, handle);
        glUnmapBuffer(target);
        glBindBuffer(target, 0);

        mapped = nullptr;
    }

    void BufferInfo::write(void *src, size_t len, size_t offset) {
        assert(valid);
        assert(offset + len <= len);

        if (mapped != nullptr) {
            memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(mapped) + offset), src, len);
        } else {
            glBindBuffer(target, handle);
            glBufferSubData(target, GLintptr(offset), GLsizeiptr(len), src);
            glBindBuffer(target, 0);
        }
    }
}
