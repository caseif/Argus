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
use crate::render_cabi;
use std::str::from_utf8_unchecked;

const fn trim_null_terminator(s: &[u8]) -> &[u8] {
    if let [rest @ .., last] = s {
        if *last == 0 {
            return rest;
        }
    }
    s
}

const fn from_c_define(str_def: &[u8]) -> &str {
    unsafe { from_utf8_unchecked(trim_null_terminator(str_def)) }
}

pub const RESOURCE_TYPE_TEXTURE_PNG: &str = from_c_define(render_cabi::RESOURCE_TYPE_TEXTURE_PNG);
pub const RESOURCE_TYPE_MATERIAL: &str = from_c_define(render_cabi::RESOURCE_TYPE_MATERIAL);
pub const RESOURCE_TYPE_SHADER_GLSL_VERT: &str =
    from_c_define(render_cabi::RESOURCE_TYPE_SHADER_GLSL_VERT);
pub const RESOURCE_TYPE_SHADER_GLSL_FRAG: &str =
    from_c_define(render_cabi::RESOURCE_TYPE_SHADER_GLSL_FRAG);

pub const LIGHTS_MAX: u32 = render_cabi::LIGHTS_MAX;

pub const SHADER_TYPE_GLSL: &str = from_c_define(render_cabi::SHADER_TYPE_GLSL);

pub const SHADER_TYPE_SPIR_V: &str = from_c_define(render_cabi::SHADER_TYPE_SPIR_V);

pub const SHADER_STRUCT_LIGHT2D_LEN: u32 = render_cabi::SHADER_STRUCT_LIGHT2D_LEN;
pub const SHADER_STRUCT_LIGHT2D_COLOR_OFF: u32 = render_cabi::SHADER_STRUCT_LIGHT2D_COLOR_OFF;
pub const SHADER_STRUCT_LIGHT2D_POSITION_OFF: u32 = render_cabi::SHADER_STRUCT_LIGHT2D_POSITION_OFF;
pub const SHADER_STRUCT_LIGHT2D_INTENSITY_OFF: u32 =
    render_cabi::SHADER_STRUCT_LIGHT2D_INTENSITY_OFF;
pub const SHADER_STRUCT_LIGHT2D_FALLOFF_GRAD_OFF: u32 =
    render_cabi::SHADER_STRUCT_LIGHT2D_FALLOFF_GRAD_OFF;
pub const SHADER_STRUCT_LIGHT2D_FALLOFF_DIST_OFF: u32 =
    render_cabi::SHADER_STRUCT_LIGHT2D_FALLOFF_DIST_OFF;
pub const SHADER_STRUCT_LIGHT2D_FALLOFF_BUFFER_OFF: u32 =
    render_cabi::SHADER_STRUCT_LIGHT2D_FALLOFF_BUFFER_OFF;
pub const SHADER_STRUCT_LIGHT2D_SHADOW_FALLOFF_GRAD_OFF: u32 =
    render_cabi::SHADER_STRUCT_LIGHT2D_SHADOW_FALLOFF_GRAD_OFF;
pub const SHADER_STRUCT_LIGHT2D_SHADOW_FALLOFF_DIST_OFF: u32 =
    render_cabi::SHADER_STRUCT_LIGHT2D_SHADOW_FALLOFF_DIST_OFF;
pub const SHADER_STRUCT_LIGHT2D_TYPE_OFF: u32 = render_cabi::SHADER_STRUCT_LIGHT2D_TYPE_OFF;
pub const SHADER_STRUCT_LIGHT2D_IS_OCCLUDABLE_OFF: u32 =
    render_cabi::SHADER_STRUCT_LIGHT2D_IS_OCCLUDABLE_OFF;

pub const SHADER_ATTRIB_POSITION: &str = from_c_define(render_cabi::SHADER_ATTRIB_POSITION);
pub const SHADER_ATTRIB_NORMAL: &str = from_c_define(render_cabi::SHADER_ATTRIB_NORMAL);
pub const SHADER_ATTRIB_COLOR: &str = from_c_define(render_cabi::SHADER_ATTRIB_COLOR);
pub const SHADER_ATTRIB_TEXCOORD: &str = from_c_define(render_cabi::SHADER_ATTRIB_TEXCOORD);
pub const SHADER_ATTRIB_ANIM_FRAME: &str = from_c_define(render_cabi::SHADER_ATTRIB_ANIM_FRAME);

pub const SHADER_OUT_COLOR: &str = from_c_define(render_cabi::SHADER_OUT_COLOR);
pub const SHADER_OUT_LIGHT_OPACITY: &str = from_c_define(render_cabi::SHADER_OUT_LIGHT_OPACITY);

pub const SHADER_UBO_GLOBAL: &str = from_c_define(render_cabi::SHADER_UBO_GLOBAL);
pub const SHADER_UBO_GLOBAL_LEN: u32 = render_cabi::SHADER_UBO_GLOBAL_LEN;
pub const SHADER_UNIFORM_GLOBAL_TIME: &str = from_c_define(render_cabi::SHADER_UNIFORM_GLOBAL_TIME);
pub const SHADER_UNIFORM_GLOBAL_TIME_OFF: u32 = render_cabi::SHADER_UNIFORM_GLOBAL_TIME_OFF;

pub const SHADER_UBO_SCENE: &str = from_c_define(render_cabi::SHADER_UBO_SCENE);
pub const SHADER_UBO_SCENE_LEN: u32 = render_cabi::SHADER_UBO_SCENE_LEN;
pub const SHADER_UNIFORM_SCENE_AL_COLOR: &str =
    from_c_define(render_cabi::SHADER_UNIFORM_SCENE_AL_COLOR);
pub const SHADER_UNIFORM_SCENE_AL_COLOR_OFF: u32 = render_cabi::SHADER_UNIFORM_SCENE_AL_COLOR_OFF;
pub const SHADER_UNIFORM_SCENE_AL_LEVEL: &str =
    from_c_define(render_cabi::SHADER_UNIFORM_SCENE_AL_LEVEL);
pub const SHADER_UNIFORM_SCENE_AL_LEVEL_OFF: u32 = render_cabi::SHADER_UNIFORM_SCENE_AL_LEVEL_OFF;
pub const SHADER_UNIFORM_SCENE_LIGHT_COUNT: &str =
    from_c_define(render_cabi::SHADER_UNIFORM_SCENE_LIGHT_COUNT);
pub const SHADER_UNIFORM_SCENE_LIGHT_COUNT_OFF: u32 =
    render_cabi::SHADER_UNIFORM_SCENE_LIGHT_COUNT_OFF;
pub const SHADER_UNIFORM_SCENE_LIGHTS: &str =
    from_c_define(render_cabi::SHADER_UNIFORM_SCENE_LIGHTS);
pub const SHADER_UNIFORM_SCENE_LIGHTS_OFF: u32 = render_cabi::SHADER_UNIFORM_SCENE_LIGHTS_OFF;

pub const SHADER_UBO_VIEWPORT: &str = from_c_define(render_cabi::SHADER_UBO_VIEWPORT);
pub const SHADER_UBO_VIEWPORT_LEN: u32 = render_cabi::SHADER_UBO_VIEWPORT_LEN;
pub const SHADER_UNIFORM_VIEWPORT_VM: &str = from_c_define(render_cabi::SHADER_UNIFORM_VIEWPORT_VM);
pub const SHADER_UNIFORM_VIEWPORT_VM_OFF: u32 = render_cabi::SHADER_UNIFORM_VIEWPORT_VM_OFF;

pub const SHADER_UBO_OBJ: &str = from_c_define(render_cabi::SHADER_UBO_OBJ);
pub const SHADER_UBO_OBJ_LEN: u32 = render_cabi::SHADER_UBO_OBJ_LEN;
pub const SHADER_UNIFORM_OBJ_UV_STRIDE: &str =
    from_c_define(render_cabi::SHADER_UNIFORM_OBJ_UV_STRIDE);
pub const SHADER_UNIFORM_OBJ_UV_STRIDE_OFF: u32 = render_cabi::SHADER_UNIFORM_OBJ_UV_STRIDE_OFF;
pub const SHADER_UNIFORM_OBJ_LIGHT_OPACITY: &str =
    from_c_define(render_cabi::SHADER_UNIFORM_OBJ_LIGHT_OPACITY);
pub const SHADER_UNIFORM_OBJ_LIGHT_OPACITY_OFF: u32 =
    render_cabi::SHADER_UNIFORM_OBJ_LIGHT_OPACITY_OFF;

pub const SHADER_STD_VERT: &str = from_c_define(render_cabi::SHADER_STD_VERT);
pub const SHADER_STD_FRAG: &str = from_c_define(render_cabi::SHADER_STD_FRAG);

pub const SHADER_SHADOWMAP_VERT: &str = from_c_define(render_cabi::SHADER_SHADOWMAP_VERT);
pub const SHADER_SHADOWMAP_FRAG: &str = from_c_define(render_cabi::SHADER_SHADOWMAP_FRAG);

pub const SHADER_LIGHTING_VERT: &str = from_c_define(render_cabi::SHADER_LIGHTING_VERT);
pub const SHADER_LIGHTING_FRAG: &str = from_c_define(render_cabi::SHADER_LIGHTING_FRAG);

pub const SHADER_LIGHTMAP_COMPOSITE_VERT: &str =
    from_c_define(render_cabi::SHADER_LIGHTMAP_COMPOSITE_VERT);
pub const SHADER_LIGHTMAP_COMPOSITE_FRAG: &str =
    from_c_define(render_cabi::SHADER_LIGHTMAP_COMPOSITE_FRAG);
