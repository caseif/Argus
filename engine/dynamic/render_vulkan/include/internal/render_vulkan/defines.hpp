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

#include "vulkan/vulkan.h"

#define BACKEND_ID "vulkan"

#define BINDING_INDEX_VBO 0
#define BINDING_INDEX_ANIM_FRAME_BUF 1

#define SHADER_ATTRIB_POSITION_LEN 2
#define SHADER_ATTRIB_NORMAL_LEN 2
#define SHADER_ATTRIB_COLOR_LEN 4
#define SHADER_ATTRIB_TEXCOORD_LEN 2
#define SHADER_ATTRIB_ANIM_FRAME_LEN 2

#define SHADER_ATTRIB_POSITION_FORMAT VK_FORMAT_R32G32_SFLOAT
#define SHADER_ATTRIB_NORMAL_FORMAT VK_FORMAT_R32G32_SFLOAT
#define SHADER_ATTRIB_COLOR_FORMAT VK_FORMAT_R32G32B32A32_SFLOAT
#define SHADER_ATTRIB_TEXCOORD_FORMAT VK_FORMAT_R32G32_SFLOAT
#define SHADER_ATTRIB_ANIM_FRAME_FORMAT VK_FORMAT_R32G32_SFLOAT

#define SHADER_OUT_COLOR_LOC 0
#define SHADER_OUT_LIGHT_OPACITY_LOC 1

#define FB_SHADER_VERT_PATH "argus:shader/vk/framebuffer_vert"
#define FB_SHADER_FRAG_PATH "argus:shader/vk/framebuffer_frag"

#define MAX_FRAMES_IN_FLIGHT 2
