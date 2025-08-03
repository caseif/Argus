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
use crate::util::buffer::GlBuffer;
use crate::util::gl_util::{GlBufferHandle, GlTextureHandle};

#[derive(Default)]
pub(crate) struct ViewportBuffers {
    pub(crate) ubo: Option<GlBuffer>,

    pub(crate) fb_primary: Option<GlBufferHandle>,
    pub(crate) fb_secondary: Option<GlBufferHandle>,
    pub(crate) fb_aux: Option<GlBufferHandle>,
    pub(crate) fb_lightmap: Option<GlBufferHandle>,

    pub(crate) color_buf_primary: Option<GlTextureHandle>,
    pub(crate) color_buf_secondary: Option<GlTextureHandle>,
    // alias of either primary or secondary color buf depending on how many
    // ping-pongs took place
    pub(crate) color_buf_front: Option<GlTextureHandle>,

    pub(crate) light_opac_map_buf: Option<GlTextureHandle>,
    pub(crate) shadowmap_buffer: Option<GlBuffer>,
    pub(crate) shadowmap_texture: Option<GlTextureHandle>,
    pub(crate) lightmap_buf: Option<GlTextureHandle>,
}

pub(crate) struct ViewportState {
    pub(crate) buffers: ViewportBuffers,
}

impl ViewportState {
    pub(crate) fn new() -> Self {
        Self {
            buffers: Default::default(),
        }
    }
}
