/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <map>

#include <cstddef>

class GLFWwindow;

namespace argus {
    // forward declarations
    class Renderer;
    class RendererImpl;
    class Window;

    extern bool g_render_module_initialized;

    extern RendererImpl *g_renderer_impl;

    extern std::map<GLFWwindow*, Window*> g_window_map;
    extern size_t g_window_count;

    extern std::map<Window*, Renderer*> g_renderer_map;

    RendererImpl &get_renderer_impl(void);
}
