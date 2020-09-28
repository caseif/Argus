/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#define MODULES_DIR_NAME "modules"
#ifdef _WIN32
    #define SHARED_LIB_PREFIX ""
    #define SHARED_LIB_EXT "dll"
#elif defined(__APPLE__)
    #define SHARED_LIB_PREFIX ""
    #define SHARED_LIB_EXT "dylib"
#else
    #define SHARED_LIB_PREFIX "lib"
    #define SHARED_LIB_EXT "so"
#endif

#define US_PER_S 1000000LLU
#define SLEEP_OVERHEAD_NS 120000LLU

#define RENDER_MODULE_OPENGL "argus_render_opengl"
#define RENDER_MODULE_OPENGLES "argus_render_opengles"
#define RENDER_MODULE_VULKAN "argus_render_vulkan"
