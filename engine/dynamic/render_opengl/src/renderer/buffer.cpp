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

#include "argus/lowlevel/debug.hpp"

#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/renderer/buffer.hpp"

#include "aglet/aglet.h"

#include <cstddef>
#include <cstring>

namespace argus {
    BufferInfo BufferInfo::create(GLenum target, size_t size, GLenum usage, bool allow_mapping,
            bool map_nonpersistent) {
        buffer_handle_t handle;
        void *mapped = nullptr;
        bool persistent = false;

        if (AGLET_GL_ARB_direct_state_access) {
            glCreateBuffers(1, &handle);
            if (AGLET_GL_ARB_buffer_storage) {
                glNamedBufferStorage(handle, GLsizeiptr(size), nullptr,
                        allow_mapping ? GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT : GL_DYNAMIC_STORAGE_BIT);
                if (allow_mapping) {
                    mapped = glMapNamedBufferRange(handle, 0, GLsizeiptr(size),
                            GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
                    persistent = true;
                }
            } else {
                glNamedBufferData(handle, GLsizeiptr(size), nullptr, usage);
                if (allow_mapping && map_nonpersistent) {
                    mapped = glMapNamedBuffer(handle, GL_WRITE_ONLY);
                }
            }
        } else {
            glGenBuffers(1, &handle);
            glBindBuffer(target, handle);
            glBufferData(target, GLsizeiptr(size), nullptr, usage);
            if (allow_mapping && map_nonpersistent) {
                mapped = glMapBuffer(target, GL_WRITE_ONLY);
            }
        }

        return BufferInfo { true, size, target, handle, mapped, allow_mapping, persistent };
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
        assert(allow_mapping);

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
        assert(allow_mapping);

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

    void BufferInfo::write(void *src, size_t len, size_t offset) {
        assert(valid);
        assert(offset + len <= this->size);

        if (mapped != nullptr) {
            memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(mapped) + offset), src, len);
        } else {
            if (AGLET_GL_ARB_direct_state_access) {
                glNamedBufferSubData(handle, GLintptr(offset), GLsizeiptr(len), src);
            } else {
                glBindBuffer(target, handle);
                glBufferSubData(target, GLintptr(offset), GLsizeiptr(len), src);
                glBindBuffer(target, 0);
            }
        }
    }

    void BufferInfo::clear(uint32_t value) {
        bool must_remap = false;

        if (!AGLET_GL_ARB_direct_state_access) {
            glBindBuffer(target, handle);
        }

        if (!AGLET_GL_ARB_buffer_storage && mapped != nullptr) {
            if (AGLET_GL_ARB_direct_state_access) {
                glUnmapNamedBuffer(handle);
            } else {
                glUnmapBuffer(target);
            }
            must_remap = true;
        }

        if (AGLET_GL_VERSION_4_3) {
            if (AGLET_GL_ARB_direct_state_access) {
                glClearNamedBufferData(handle, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &value);
            } else {
                glClearBufferData(target, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &value);
            }
        } else {
            if (AGLET_GL_ARB_direct_state_access) {
                glClearNamedBufferSubData(handle, GL_R32UI, 0, GLsizeiptr(size), GL_RED_INTEGER, GL_UNSIGNED_INT,
                        &value);
            } else {
                glClearBufferSubData(target, GL_R32UI, 0, GLsizeiptr(size), GL_RED_INTEGER, GL_UNSIGNED_INT, &value);
            }
        }

        if (must_remap) {
            if (AGLET_GL_ARB_direct_state_access) {
                glMapNamedBuffer(handle, GL_WRITE_ONLY);
            } else {
                glMapBuffer(target, GL_WRITE_ONLY);
            }
        }

        if (!AGLET_GL_ARB_direct_state_access) {
            glBindBuffer(target, 0);
        }
    }
}
