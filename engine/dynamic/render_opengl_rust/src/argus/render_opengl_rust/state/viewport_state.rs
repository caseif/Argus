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
use render_rustabi::argus::render::{AttachedViewport, Matrix4x4};
use crate::argus::render_opengl_rust::util::buffer::GlBuffer;
use crate::argus::render_opengl_rust::util::gl_util::{GlBufferHandle, GlTextureHandle};

#[derive(Default)]
struct ViewportBuffers {
    ubo: Option<GlBuffer>,

    fb_primary: Option<GlBufferHandle>,
    fb_secondary: Option<GlBufferHandle>,
    fb_aux: Option<GlBufferHandle>,
    fb_lightmap: Option<GlBufferHandle>,

    color_buf_primary: Option<GlTextureHandle>,
    color_buf_secondary: Option<GlTextureHandle>,
    // alias of either primary or secondary color buf depending on how many
    // ping-pongs took place
    color_buf_front: Option<GlTextureHandle>,

    light_opac_map_buf: Option<GlTextureHandle>,
    shadowmap_buffer: Option<GlBuffer>,
    shadowmap_texture: Option<GlTextureHandle>,
    lightmap_buf: Option<GlTextureHandle>,
}

pub(crate) struct ViewportState {
    viewport: AttachedViewport,
    view_matrix: Matrix4x4,
    view_matrix_dirty: bool,
    buffers: ViewportBuffers,
}

impl ViewportState {
    pub(crate) fn new(viewport: AttachedViewport) -> Self {
        Self {
            viewport,
            view_matrix: Default::default(),
            view_matrix_dirty: true,
            buffers: Default::default(),
        }
    }
}
