/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#define SHADER_ATTRIB_POSITION "in_Position"
#define SHADER_ATTRIB_NORMAL "in_Normal"
#define SHADER_ATTRIB_COLOR "in_Color"
#define SHADER_ATTRIB_TEXCOORD "in_TexCoord"
#define SHADER_ATTRIB_ANIM_FRAME "in_AnimFrame"

#define SHADER_OUT_COLOR "out_Color"

#define SHADER_UBO_GLOBAL "Global"
#define SHADER_UBO_GLOBAL_LEN 16
#define SHADER_UNIFORM_GLOBAL_TIME "Time"
#define SHADER_UNIFORM_GLOBAL_TIME_OFF 0

#define SHADER_UBO_VIEWPORT "Viewport"
#define SHADER_UBO_VIEWPORT_LEN 64
#define SHADER_UNIFORM_VIEWPORT_VM "ViewMatrix"
#define SHADER_UNIFORM_VIEWPORT_VM_OFF 0

#define SHADER_UBO_OBJ "Object"
#define SHADER_UBO_OBJ_LEN 16
#define SHADER_UNIFORM_OBJ_UV_STRIDE "UvStride"
#define SHADER_UNIFORM_OBJ_UV_STRIDE_OFF 0

#define RESOURCE_TYPE_SHADER_GLSL_VERT "text/x-glsl-vertex"
#define RESOURCE_TYPE_SHADER_GLSL_FRAG "text/x-glsl-fragment"
