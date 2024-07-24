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

#pragma once

#define RESOURCE_TYPE_TEXTURE_PNG "image/png"
#define RESOURCE_TYPE_MATERIAL "application/x-argus-material+json"
#define RESOURCE_TYPE_SHADER_GLSL_VERT "text/x-glsl-vertex"
#define RESOURCE_TYPE_SHADER_GLSL_FRAG "text/x-glsl-fragment"

#define LIGHTS_MAX 32U

#define SHADER_STRUCT_LIGHT2D_LEN 64U
#define SHADER_STRUCT_LIGHT2D_COLOR_OFF 0U
#define SHADER_STRUCT_LIGHT2D_POSITION_OFF 16U
#define SHADER_STRUCT_LIGHT2D_INTENSITY_OFF 32U
#define SHADER_STRUCT_LIGHT2D_FALLOFF_GRAD_OFF 36U
#define SHADER_STRUCT_LIGHT2D_FALLOFF_DIST_OFF 40U
#define SHADER_STRUCT_LIGHT2D_FALLOFF_BUFFER_OFF 44U
#define SHADER_STRUCT_LIGHT2D_SHADOW_FALLOFF_GRAD_OFF 48U
#define SHADER_STRUCT_LIGHT2D_SHADOW_FALLOFF_DIST_OFF 52U
#define SHADER_STRUCT_LIGHT2D_TYPE_OFF 56U
#define SHADER_STRUCT_LIGHT2D_IS_OCCLUDABLE_OFF 60U

#define SHADER_ATTRIB_POSITION "in_Position"
#define SHADER_ATTRIB_NORMAL "in_Normal"
#define SHADER_ATTRIB_COLOR "in_Color"
#define SHADER_ATTRIB_TEXCOORD "in_TexCoord"
#define SHADER_ATTRIB_ANIM_FRAME "in_AnimFrame"

#define SHADER_OUT_COLOR "out_Color"
#define SHADER_OUT_LIGHT_OPACITY "out_LightOpacity"

#define SHADER_UBO_GLOBAL "Global"
#define SHADER_UBO_GLOBAL_LEN 16U
#define SHADER_UNIFORM_GLOBAL_TIME "Time"
#define SHADER_UNIFORM_GLOBAL_TIME_OFF 0U

#define SHADER_UBO_SCENE "Scene"
#define SHADER_UBO_SCENE_LEN (48U + (SHADER_STRUCT_LIGHT2D_LEN * LIGHTS_MAX))
#define SHADER_UNIFORM_SCENE_AL_COLOR "AmbientLightColor"
#define SHADER_UNIFORM_SCENE_AL_COLOR_OFF 0U
#define SHADER_UNIFORM_SCENE_AL_LEVEL "AmbientLightLevel"
#define SHADER_UNIFORM_SCENE_AL_LEVEL_OFF 16U
#define SHADER_UNIFORM_SCENE_LIGHT_COUNT "LightCount"
#define SHADER_UNIFORM_SCENE_LIGHT_COUNT_OFF 20U
#define SHADER_UNIFORM_SCENE_LIGHTS "Lights"
#define SHADER_UNIFORM_SCENE_LIGHTS_OFF 32U

#define SHADER_UBO_VIEWPORT "Viewport"
#define SHADER_UBO_VIEWPORT_LEN 64U
#define SHADER_UNIFORM_VIEWPORT_VM "ViewMatrix"
#define SHADER_UNIFORM_VIEWPORT_VM_OFF 0U

#define SHADER_UBO_OBJ "Object"
#define SHADER_UBO_OBJ_LEN 16U
#define SHADER_UNIFORM_OBJ_UV_STRIDE "UvStride"
#define SHADER_UNIFORM_OBJ_UV_STRIDE_OFF 0U
#define SHADER_UNIFORM_OBJ_LIGHT_OPACITY "LightOpacity"
#define SHADER_UNIFORM_OBJ_LIGHT_OPACITY_OFF 8U

#define SHADER_STD_VERT "argus:render/shader/std_vert"
#define SHADER_STD_FRAG "argus:render/shader/std_frag"

#define SHADER_SHADOWMAP_VERT "argus:render/shader/shadowmap_vert"
#define SHADER_SHADOWMAP_FRAG "argus:render/shader/shadowmap_frag"

#define SHADER_LIGHTING_VERT "argus:render/shader/lighting_vert"
#define SHADER_LIGHTING_FRAG "argus:render/shader/lighting_frag"

#define SHADER_LIGHTMAP_COMPOSITE_VERT "argus:render/shader/lightmap_composite_vert"
#define SHADER_LIGHTMAP_COMPOSITE_FRAG "argus:render/shader/lightmap_composite_frag"
