/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#define RESOURCE_TYPE_TEXTURE_PNG "image/png"
#define RESOURCE_TYPE_SHADER_GLSL_VERT "text/x-glsl-vert"
#define RESOURCE_TYPE_SHADER_GLSL_FRAG "text/x-glsl-frag"
#define RESOURCE_TYPE_MATERIAL "text/x-argus-material+json"

#define MODULE_RENDER_OPENGL "render_opengl"
#define MODULE_RENDER_OPENGLES "render_opengles"
#define MODULE_RENDER_VULKAN "render_vulkan"

#define FN_CREATE_OPENGL_BACKEND "create_opengl_backend"
#define FN_CREATE_OPENGLES_BACKEND "create_opengles_backend"
#define FN_CREATE_VULKAN_BACKEND "create_vulkan_backend"
