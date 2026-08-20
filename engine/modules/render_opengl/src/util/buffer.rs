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

use crate::aglet::*;
use crate::util::gl_util::*;

use std::{ptr, slice};
use crate::util::support::{GlExt, GlSupport};

pub(crate) struct GlBuffer {
    size: usize,
    target: GLenum,
    handle: GlBufferHandle,
    mapped: Option<*mut u8>,
    allow_mapping: bool,
    persistent: bool,
}

impl GlBuffer {
    pub(crate) fn new(
        target: GLenum,
        size: usize,
        usage: GLenum,
        allow_mapping: bool,
        map_nonpersistent: bool,
    ) -> Self {
        let mut handle: GlBufferHandle = 0;
        let mut mapped: Option<*mut u8> = None;
        let mut persistent = false;

        if GlSupport::have(GlExt::DirectStateAccess) {
            glCreateBuffers(1, &mut handle);
            if GlSupport::have(GlExt::BufferStorage) {
                let storage_flags = if allow_mapping {
                    GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT
                } else {
                    GL_DYNAMIC_STORAGE_BIT
                };
                glNamedBufferStorage(handle, size as GLsizeiptr, ptr::null(), storage_flags);
                if allow_mapping {
                    mapped = Some(
                        glMapNamedBufferRange(
                            handle,
                            0,
                            size as GLsizeiptr,
                            GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
                        )
                        .cast(),
                    );
                    persistent = true;
                }
            } else {
                glNamedBufferData(handle, size as GLsizeiptr, ptr::null(), usage);
                if allow_mapping && map_nonpersistent {
                    mapped = Some(glMapNamedBuffer(handle, GL_WRITE_ONLY).cast());
                }
            }
        } else {
            glGenBuffers(1, &mut handle);
            glBindBuffer(target, handle);
            glBufferData(target, size as GLsizeiptr, ptr::null(), usage);
            if allow_mapping && map_nonpersistent {
                mapped = Some(glMapBuffer(target, GL_WRITE_ONLY).cast());
            }
        }

        Self {
            size,
            target,
            handle,
            mapped,
            allow_mapping,
            persistent,
        }
    }

    pub(crate) fn get_handle(&self) -> GlBufferHandle {
        self.handle
    }

    #[allow(unused)]
    pub(crate) fn map_write(&mut self) {
        assert!(self.allow_mapping);

        if self.persistent {
            return;
        }

        assert!(self.mapped.is_none());

        if GlSupport::have(GlExt::DirectStateAccess) {
            self.mapped = Some(glMapNamedBuffer(self.handle, GL_WRITE_ONLY).cast());
        } else {
            glBindBuffer(self.target, self.handle);
            self.mapped = Some(glMapBuffer(self.target, GL_WRITE_ONLY).cast());
            glBindBuffer(self.target, 0);
        }
    }

    pub(crate) fn unmap(&mut self, force: bool) {
        assert!(self.allow_mapping);

        if self.persistent && !force {
            return;
        }

        assert!(self.mapped.is_some());

        if GlSupport::have(GlExt::DirectStateAccess) {
            glUnmapNamedBuffer(self.handle);
        } else {
            glBindBuffer(self.target, self.handle);
            glUnmapBuffer(self.target);
            glBindBuffer(self.target, 0);
        }

        self.mapped = None;
    }

    pub(crate) fn write_vals<T>(&self, src: &[T], offset: usize) {
        let len = size_of_val(src);

        assert!(offset + len <= self.size);

        match self.mapped {
            Some(mapped_ptr) => {
                unsafe { mapped_ptr.add(offset).copy_from(src.as_ptr().cast(), len) };
            },
            None => {
                if GlSupport::have(GlExt::DirectStateAccess) {
                    glNamedBufferSubData(
                        self.handle,
                        offset as GLintptr,
                        len as GLsizeiptr,
                        src.as_ptr().cast(),
                    );
                } else {
                    glBindBuffer(self.target, self.handle);
                    glBufferSubData(
                        self.target,
                        offset as GLintptr,
                        len as GLsizeiptr,
                        src.as_ptr().cast(),
                    );
                    glBindBuffer(self.target, 0);
                }
            }
        }
    }

    pub(crate) fn write_val<T>(&self, val: &T, offset: usize) {
        self.write_vals(
            unsafe { slice::from_raw_parts((val as *const T) as *const u8, size_of_val(val)) },
            offset,
        );
    }

    pub(crate) fn clear(&self, value: u32) {
        let mut must_remap = false;

        if !GlSupport::have(GlExt::DirectStateAccess) {
            glBindBuffer(self.target, self.handle);
        }

        if !GlSupport::have(GlExt::BufferStorage) && self.mapped.is_some() {
            if GlSupport::have(GlExt::DirectStateAccess) {
                glUnmapNamedBuffer(self.handle);
            } else {
                glUnmapBuffer(self.target);
            }
            must_remap = true;
        }

        if GlSupport::have(GlExt::ClearBufferObject) {
            if GlSupport::have(GlExt::DirectStateAccess) {
                glClearNamedBufferData(
                    self.handle,
                    GL_R32UI,
                    GL_RED_INTEGER,
                    GL_UNSIGNED_INT,
                    ptr::addr_of!(value).cast(),
                );
            } else {
                glClearBufferData(
                    self.target,
                    GL_R32UI,
                    GL_RED_INTEGER,
                    GL_UNSIGNED_INT,
                    ptr::addr_of!(value).cast(),
                );
            }
        } else {
            let zeroes = vec![0u8; value as usize * size_of::<u32>()];
            if GlSupport::have(GlExt::DirectStateAccess) {
                glNamedBufferSubData(
                    self.handle,
                    0,
                    self.size as GLsizeiptr,
                    zeroes.as_ptr().cast(),
                );
            } else {
                glBufferSubData(
                    self.target,
                    0,
                    self.size as GLsizeiptr,
                    zeroes.as_ptr().cast(),
                );
            }
        }

        if must_remap {
            if GlSupport::have(GlExt::DirectStateAccess) {
                glMapNamedBuffer(self.handle, GL_WRITE_ONLY);
            } else {
                glMapBuffer(self.target, GL_WRITE_ONLY);
            }
        }

        if !GlSupport::have(GlExt::DirectStateAccess) {
            glBindBuffer(self.target, 0);
        }
    }
}

impl Drop for GlBuffer {
    fn drop(&mut self) {
        if self.allow_mapping {
            self.unmap(true);
        }

        glDeleteBuffers(0, &self.handle);

        self.handle = 0;
    }
}
