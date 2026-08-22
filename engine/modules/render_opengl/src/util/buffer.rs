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
use std::borrow::Borrow;
use std::cell::RefCell;
use crate::util::support::{GlExt, GlSupport};

pub(crate) struct GlBuffer {
    size: usize,
    target: GLenum,
    handle: GlBufferHandle,
    mapped: RefCell<Option<*mut u8>>,
    allow_mapping: bool,
    persistent: bool,
}

impl GlBuffer {
    pub(crate) fn new(
        target: GLenum,
        size: usize,
        usage: GLenum,
        allow_mapping: bool,
    ) -> Self {
        let mut handle: GlBufferHandle = 0;
        let mapped: RefCell<Option<*mut u8>> = RefCell::new(None);
        let mut persistent = false;

        if GlSupport::have(GlExt::DirectStateAccess) {
            glCreateBuffers(1, &mut handle);
        } else {
            glGenBuffers(1, &mut handle);
            glBindBuffer(target, handle);
        }

        if GlSupport::have(GlExt::BufferStorage) {
            let storage_flags = if allow_mapping {
                GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT
            } else {
                GL_DYNAMIC_STORAGE_BIT
            };
            if GlSupport::have(GlExt::DirectStateAccess) {
                glNamedBufferStorage(handle, size as GLsizeiptr, ptr::null(), storage_flags);
            } else {
                glBufferStorage(target, size as GLsizeiptr, ptr::null(), storage_flags);
            }

            if allow_mapping {
                mapped.replace(Some(
                    if GlSupport::have(GlExt::DirectStateAccess) {
                        glMapNamedBufferRange(
                            handle,
                            0,
                            size as GLsizeiptr,
                            GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
                        )
                    } else {
                        glMapBufferRange(
                            target,
                            0,
                            size as GLsizeiptr,
                            GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
                        )
                    }.cast(),
                ));
                persistent = true;
            }
        } else {
            if GlSupport::have(GlExt::DirectStateAccess) {
                glNamedBufferData(handle, size as GLsizeiptr, ptr::null(), usage);
            } else {
                glBufferData(target, size as GLsizeiptr, ptr::null(), usage);
            }
            persistent = false;
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

    #[must_use]
    pub(crate) fn map_write(&'_ self) -> GlBufferMapGuard<'_> {
        assert!(self.allow_mapping);

        if self.persistent {
            return GlBufferMapGuard::Persistent;
        }

        assert!(self.mapped.borrow().is_none());

        if GlSupport::have(GlExt::DirectStateAccess) {
            self.mapped.replace(Some(glMapNamedBuffer(self.handle, GL_WRITE_ONLY).cast()));
        } else {
            glBindBuffer(self.target, self.handle);
            self.mapped.replace(Some(glMapBuffer(self.target, GL_WRITE_ONLY).cast()));
            glBindBuffer(self.target, 0);
        }

        GlBufferMapGuard::Transient(self)
    }

    fn unmap(&mut self, force: bool) {
        assert!(self.allow_mapping);

        if self.persistent && !force {
            return;
        }

        assert!(self.mapped.borrow().is_some());

        if GlSupport::have(GlExt::DirectStateAccess) {
            glUnmapNamedBuffer(self.handle);
        } else {
            glBindBuffer(self.target, self.handle);
            glUnmapBuffer(self.target);
            glBindBuffer(self.target, 0);
        }

        self.mapped.replace(None);
    }

    pub(crate) fn write_vals<T>(&self, src: &[T], offset: usize) {
        let len = size_of_val(src);

        assert!(offset + len <= self.size);

        match *self.mapped.borrow() {
            Some(mapped_ptr) => {
                if mapped_ptr.is_null() {
                    panic!("Mapped GL buffer pointer is null!");
                }
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

    pub(crate) fn write_val<T>(&self, val: impl Borrow<T>, offset: usize) {
        self.write_vals(
            unsafe {
                slice::from_raw_parts(
                    (val.borrow() as *const T) as *const u8,
                    size_of_val(val.borrow()),
                )
            },
            offset,
        );
    }

    pub(crate) fn clear(&self, value: u32) {
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
            let clear_data = vec![value; (self.size + (size_of::<u32>() - 1)) / size_of::<u32>()];
            let clear_data_bytes = unsafe {
                slice::from_raw_parts(clear_data.as_ptr().cast::<u8>(), self.size)
            };
            self.write_vals(&clear_data_bytes, 0);
        }
    }
}

impl Drop for GlBuffer {
    fn drop(&mut self) {
        if self.allow_mapping && self.mapped.borrow().is_some() {
            self.unmap(true);
        }

        glDeleteBuffers(1, &self.handle);

        self.handle = 0;
    }
}

pub(crate) enum GlBufferMapGuard<'a> {
    Persistent,
    Transient(&'a GlBuffer),
}

impl<'a> Drop for GlBufferMapGuard<'a> {
    fn drop(&mut self) {
        if let GlBufferMapGuard::Transient(buffer) = self {
            if GlSupport::have(GlExt::DirectStateAccess) {
                glUnmapNamedBuffer(buffer.handle);
            } else {
                glBindBuffer(buffer.target, buffer.handle);
                glUnmapBuffer(buffer.target);
                glBindBuffer(buffer.target, 0);
            }
            buffer.mapped.replace(None);
        }
    }
}
