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

#define SHADER_ATTRIB_OUT_FRAGDATA "out_Color"

#define SHADER_UNIFORM_VIEW_MATRIX "u_ViewMatrix"
#define SHADER_UNIFORM_TIME "u_Time"
#define SHADER_UNIFORM_UV_STRIDE "u_UvStride"

#define RESOURCE_TYPE_SHADER_GLSL_VERT "text/x-glsl-vertex"
#define RESOURCE_TYPE_SHADER_GLSL_FRAG "text/x-glsl-fragment"
