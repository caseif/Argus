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
use std::slice;
use crate::argus::render::Vertex2d;
use crate::render_cabi::*;

pub struct RenderPrimitive2d {
    pub vertices: Vec<Vertex2d>,
}

impl From<&RenderPrimitive2d> for ArgusRenderPrimitive2d {
    fn from(value: &RenderPrimitive2d) -> Self {
        ArgusRenderPrimitive2d {
            vertices: value.vertices.as_ptr().cast(),
            vertex_count: value.vertices.len(),
        }
    }
}

impl From<ArgusRenderPrimitive2d> for RenderPrimitive2d {
    fn from(prim: ArgusRenderPrimitive2d) -> Self {
        unsafe {
            let s = slice::from_raw_parts(prim.vertices.cast(), prim.vertex_count);
            Self {
                vertices: s.to_vec()
            }
        }
    }
}
