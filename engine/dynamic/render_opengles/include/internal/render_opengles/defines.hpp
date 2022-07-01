/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#define BACKEND_ID "opengl_es"

#define _VERTEX_POSITION_LEN 2
#define _VERTEX_NORMAL_LEN 2
#define _VERTEX_COLOR_LEN 4
#define _VERTEX_TEXCOORD_LEN 2
#define _VERTEX_WORD_LEN sizeof(GLfloat)

#define _GL_LOG_MAX_LEN 255

#define SHADER_ATTRIB_IN_POSITION "in_Position"
#define SHADER_ATTRIB_IN_NORMAL "in_Normal"
#define SHADER_ATTRIB_IN_COLOR "in_Color"
#define SHADER_ATTRIB_IN_TEXCOORD "in_TexCoord"

#define SHADER_ATTRIB_IN_POSITION_LEN 2
#define SHADER_ATTRIB_IN_NORMAL_LEN 2
#define SHADER_ATTRIB_IN_COLOR_LEN 4
#define SHADER_ATTRIB_IN_TEXCOORD_LEN 2

#define SHADER_UNIFORM_VIEW_MATRIX "uniform_ViewMat"

#define RESOURCE_TYPE_SHADER_GLSL_VERT "text/x-glsl-vertex"
#define RESOURCE_TYPE_SHADER_GLSL_FRAG "text/x-glsl-fragment"

#define FB_SHADER_VERT_PATH "argus:shader/gles/framebuffer_vert"
#define FB_SHADER_FRAG_PATH "argus:shader/gles/framebuffer_frag"
