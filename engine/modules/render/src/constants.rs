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

pub const RESOURCE_TYPE_TEXTURE_PNG: &str = "image/png";
pub const RESOURCE_TYPE_MATERIAL: &str = "application/x-argus-material+json";
pub const RESOURCE_TYPE_SHADER_GLSL_VERT: &str = "text/x-glsl-vertex";
pub const RESOURCE_TYPE_SHADER_GLSL_FRAG: &str = "text/x-glsl-fragment";

pub const LIGHTS_MAX: u32 = 32;

pub const SHADER_TYPE_GLSL: &str = "glsl";
pub const SHADER_TYPE_SPIR_V: &str = "spirv";

pub const SHADER_STRUCT_LIGHT2D_LEN: u32 = 64;
pub const SHADER_STRUCT_LIGHT2D_COLOR_OFF: u32 = 0;
pub const SHADER_STRUCT_LIGHT2D_POSITION_OFF: u32 = 16;
pub const SHADER_STRUCT_LIGHT2D_INTENSITY_OFF: u32 = 32;
pub const SHADER_STRUCT_LIGHT2D_FALLOFF_GRAD_OFF: u32 = 36;
pub const SHADER_STRUCT_LIGHT2D_FALLOFF_DIST_OFF: u32 = 40;
pub const SHADER_STRUCT_LIGHT2D_FALLOFF_BUFFER_OFF: u32 = 44;
pub const SHADER_STRUCT_LIGHT2D_SHADOW_FALLOFF_GRAD_OFF: u32 = 48;
pub const SHADER_STRUCT_LIGHT2D_SHADOW_FALLOFF_DIST_OFF: u32 = 52;
pub const SHADER_STRUCT_LIGHT2D_TYPE_OFF: u32 = 56;
pub const SHADER_STRUCT_LIGHT2D_IS_OCCLUDABLE_OFF: u32 = 60;

pub const SHADER_ATTRIB_POSITION: &str = "in_Position";
pub const SHADER_ATTRIB_NORMAL: &str = "in_Normal";
pub const SHADER_ATTRIB_COLOR: &str = "in_Color";
pub const SHADER_ATTRIB_TEXCOORD: &str = "in_TexCoord";
pub const SHADER_ATTRIB_ANIM_FRAME: &str = "in_AnimFrame";

pub const SHADER_OUT_COLOR: &str = "out_Color";
pub const SHADER_OUT_LIGHT_OPACITY: &str = "out_LightOpacity";

pub const SHADER_UBO_GLOBAL: &str = "Global";
pub const SHADER_UBO_GLOBAL_LEN: u32 = 16;
pub const SHADER_UNIFORM_GLOBAL_TIME: &str = "Time";
pub const SHADER_UNIFORM_GLOBAL_TIME_OFF: u32 = 0;

pub const SHADER_UBO_SCENE: &str = "Scene";
pub const SHADER_UBO_SCENE_LEN: u32 = 48 + (SHADER_STRUCT_LIGHT2D_LEN * LIGHTS_MAX);
pub const SHADER_UNIFORM_SCENE_AL_COLOR: &str = "AmbientLightColor";
pub const SHADER_UNIFORM_SCENE_AL_COLOR_OFF: u32 = 0;
pub const SHADER_UNIFORM_SCENE_AL_LEVEL: &str = "AmbientLightLevel";
pub const SHADER_UNIFORM_SCENE_AL_LEVEL_OFF: u32 = 16;
pub const SHADER_UNIFORM_SCENE_LIGHT_COUNT: &str = "LightCount";
pub const SHADER_UNIFORM_SCENE_LIGHT_COUNT_OFF: u32 = 20;
pub const SHADER_UNIFORM_SCENE_LIGHTS: &str = "Lights";
pub const SHADER_UNIFORM_SCENE_LIGHTS_OFF: u32 = 32;

pub const SHADER_UBO_VIEWPORT: &str = "Viewport";
pub const SHADER_UBO_VIEWPORT_LEN: u32 = 64;
pub const SHADER_UNIFORM_VIEWPORT_VM: &str = "ViewMatrix";
pub const SHADER_UNIFORM_VIEWPORT_VM_OFF: u32 = 0;

pub const SHADER_UBO_OBJ: &str = "Object";
pub const SHADER_UBO_OBJ_LEN: u32 = 16;
pub const SHADER_UNIFORM_OBJ_UV_STRIDE: &str = "UvStride";
pub const SHADER_UNIFORM_OBJ_UV_STRIDE_OFF: u32 = 0;
pub const SHADER_UNIFORM_OBJ_LIGHT_OPACITY: &str = "LightOpacity";
pub const SHADER_UNIFORM_OBJ_LIGHT_OPACITY_OFF: u32 = 8;

pub const SHADER_STD_VERT: &str = "argus:render/shader/std_vert";
pub const SHADER_STD_FRAG: &str = "argus:render/shader/std_frag";

pub const SHADER_SHADOWMAP_VERT: &str = "argus:render/shader/shadowmap_vert";
pub const SHADER_SHADOWMAP_FRAG: &str = "argus:render/shader/shadowmap_frag";

pub const SHADER_LIGHTING_VERT: &str = "argus:render/shader/lighting_vert";
pub const SHADER_LIGHTING_FRAG: &str = "argus:render/shader/lighting_frag";

pub const SHADER_LIGHTMAP_COMPOSITE_VERT: &str = "argus:render/shader/lightmap_composite_vert";
pub const SHADER_LIGHTMAP_COMPOSITE_FRAG: &str = "argus:render/shader/lightmap_composite_frag";
