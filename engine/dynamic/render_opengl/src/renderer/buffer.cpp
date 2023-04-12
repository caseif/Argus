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
    BufferInfo create_buffer(GLenum target, size_t len, GLenum usage, bool map_nonpersistent) {
        buffer_handle_t buffer;
        void *mapped = nullptr;
        bool persistent = false;

        if (AGLET_GL_ARB_direct_state_access) {
            glCreateBuffers(1, &buffer);
            if (AGLET_GL_ARB_buffer_storage) {
                glNamedBufferStorage(buffer, GLsizeiptr(len), nullptr,
                        GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
                mapped = glMapNamedBufferRange(buffer, 0, GLsizeiptr(len),
                        GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
                persistent = true;
            } else {
                glNamedBufferData(buffer, GLsizeiptr(len), nullptr, usage);
                if (map_nonpersistent) {
                    mapped = glMapNamedBuffer(buffer, GL_WRITE_ONLY);
                }
            }
        } else {
            glGenBuffers(1, &buffer);
            glBindBuffer(target, buffer);
            glBufferData(target, GLsizeiptr(len), nullptr, usage);
            if (map_nonpersistent) {
                mapped = glMapBuffer(target, GL_WRITE_ONLY);
            }
        }

        return BufferInfo { len, buffer, mapped, persistent };
    }

    void map_buffer_w(BufferInfo &buffer, GLenum target) {
        assert(buffer.buffer != 0);

        if (buffer.persistent) {
            return;
        }

        assert(buffer.mapped == nullptr);

        if (AGLET_GL_ARB_direct_state_access) {
            buffer.mapped = glMapNamedBuffer(buffer.buffer, GL_WRITE_ONLY);
        } else {
            glBindBuffer(target, buffer.buffer);
            buffer.mapped = glMapBuffer(target, GL_WRITE_ONLY);
            glBindBuffer(target, 0);
        }
    }

    void unmap_buffer(BufferInfo &buffer, GLenum target) {
        assert(buffer.buffer != 0);

        if (buffer.persistent) {
            return;
        }

        assert(buffer.mapped != nullptr);

        if (AGLET_GL_ARB_direct_state_access) {
            glUnmapNamedBuffer(buffer.buffer);
        } else {
            glBindBuffer(target, buffer.buffer);
            glUnmapBuffer(target);
            glBindBuffer(target, 0);
        }

        buffer.mapped = nullptr;
    }

    void write_buffer_data(BufferInfo &buffer, size_t offset, size_t len, void *src) {
        assert(buffer.buffer != 0);
        assert(buffer.mapped != nullptr);
        assert(offset + len <= buffer.len);

        memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer.mapped) + offset), src, len);
    }
}
