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

#![allow(unused)]

use argus_render::constants::LIGHTS_MAX;
use crate::aglet::GLfloat;

pub(crate) const VERTEX_POSITION_LEN: usize = 2;
pub(crate) const VERTEX_NORMAL_LEN: usize = 2;
pub(crate) const VERTEX_COLOR_LEN: usize = 4;
pub(crate) const VERTEX_TEXCOORD_LEN: usize = 2;
pub(crate) const VERTEX_WORD_LEN: usize = size_of::<GLfloat>();

pub(crate) const GL_LOG_MAX_LEN: usize = 255;

pub(crate) const SHADER_ATTRIB_POSITION_LEN: usize = 2;
pub(crate) const SHADER_ATTRIB_NORMAL_LEN: usize = 2;
pub(crate) const SHADER_ATTRIB_COLOR_LEN: usize = 4;
pub(crate) const SHADER_ATTRIB_TEXCOORD_LEN: usize = 2;
pub(crate) const SHADER_ATTRIB_ANIM_FRAME_LEN: usize = 2;

pub(crate) const FB_SHADER_ATTRIB_POSITION_LOC: u32 = 0;
pub(crate) const FB_SHADER_ATTRIB_TEXCOORD_LOC: u32 = 1;
pub(crate) const FB_SHADER_ATTRIB_ANIM_FRAME_LOC: u32 = 2;

pub(crate) const SHADOW_RAYS_COUNT: usize = 720;

pub(crate) const SHADER_IMAGE_SHADOWMAP_LEN: usize = SHADOW_RAYS_COUNT * LIGHTS_MAX as usize * 4;

pub(crate) const FB_SHADER_VERT_PATH: &str = "argus:shader/opengl/framebuffer_vert";
pub(crate) const FB_SHADER_FRAG_PATH: &str = "argus:shader/opengl/framebuffer_frag";
