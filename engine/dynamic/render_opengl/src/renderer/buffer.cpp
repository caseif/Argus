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

#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/renderer/buffer.hpp"

#include "aglet/aglet.h"

#include <cassert>
#include <cstddef>
#include <cstring>

namespace argus {
    BufferInfo BufferInfo::create(size_t size, GLenum target, GLenum usage, bool map_nonpersistent) {
        buffer_handle_t handle;
        void *mapped = nullptr;
        bool persistent = false;

        if (AGLET_GL_ARB_direct_state_access) {
            glCreateBuffers(1, &handle);
            if (AGLET_GL_ARB_buffer_storage) {
                glNamedBufferStorage(handle, GLsizeiptr(size), nullptr,
                        GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
                mapped = glMapNamedBufferRange(handle, 0, GLsizeiptr(size),
                        GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
                persistent = true;
            } else {
                glNamedBufferData(handle, GLsizeiptr(size), nullptr, usage);
                if (map_nonpersistent) {
                    mapped = glMapNamedBuffer(handle, GL_WRITE_ONLY);
                }
            }
        } else {
            glGenBuffers(1, &handle);
            glBindBuffer(target, handle);
            glBufferData(target, GLsizeiptr(size), nullptr, usage);
            if (map_nonpersistent) {
                mapped = glMapBuffer(target, GL_WRITE_ONLY);
            }
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

        if (AGLET_GL_ARB_direct_state_access) {
            mapped = glMapNamedBuffer(handle, GL_WRITE_ONLY);
        } else {
            glBindBuffer(target, handle);
            mapped = glMapBuffer(target, GL_WRITE_ONLY);
            glBindBuffer(target, 0);
        }
    }

    void BufferInfo::unmap() {
        assert(valid);

        if (persistent) {
            return;
        }

        assert(mapped != nullptr);

        if (AGLET_GL_ARB_direct_state_access) {
            glUnmapNamedBuffer(handle);
        } else {
            glBindBuffer(target, handle);
            glUnmapBuffer(target);
            glBindBuffer(target, 0);
        }

        mapped = nullptr;
    }

    void BufferInfo::write_data(void *src, size_t len, size_t offset) {
        assert(valid);
        assert(mapped != nullptr);
        assert(offset + len <= len);

        memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(mapped) + offset), src, len);
    }
}
