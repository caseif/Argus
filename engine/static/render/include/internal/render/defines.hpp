/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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
#define RESOURCE_TYPE_MATERIAL "text/x-argus-material+json"

#define MODULE_RENDER_OPENGL "render_opengl"
#define MODULE_RENDER_OPENGLES "render_opengles"
#define MODULE_RENDER_VULKAN "render_vulkan"

#define FN_CREATE_OPENGL_BACKEND "create_opengl_backend"
#define FN_CREATE_OPENGLES_BACKEND "create_opengles_backend"
#define FN_CREATE_VULKAN_BACKEND "create_vulkan_backend"
