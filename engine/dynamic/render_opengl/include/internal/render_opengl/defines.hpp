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

#define BACKEND_ID "opengl"

#define _VERTEX_POSITION_LEN 2
#define _VERTEX_NORMAL_LEN 2
#define _VERTEX_COLOR_LEN 4
#define _VERTEX_TEXCOORD_LEN 2
#define _VERTEX_WORD_LEN sizeof(GLfloat)

#define _GL_LOG_MAX_LEN 255

#define SHADER_ATTRIB_POSITION_LEN 2
#define SHADER_ATTRIB_NORMAL_LEN 2
#define SHADER_ATTRIB_COLOR_LEN 4
#define SHADER_ATTRIB_TEXCOORD_LEN 2
#define SHADER_ATTRIB_ANIM_FRAME_LEN 2

#define FB_SHADER_ATTRIB_POSITION_LOC 0
#define FB_SHADER_ATTRIB_TEXCOORD_LOC 1
#define FB_SHADER_ATTRIB_ANIM_FRAME_LOC 2

#define FB_SHADER_VERT_PATH "argus:shader/gl/framebuffer_vert"
#define FB_SHADER_FRAG_PATH "argus:shader/gl/framebuffer_frag"
