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

#include "argus/core/module.hpp"

#include <map>

#include <cstddef>

namespace argus {
    // forward declarations
    class Renderer;
    class RendererImpl;
    class Window;

    extern bool g_render_module_initialized;

    extern RendererImpl *g_renderer_impl;

    extern std::map<Window*, Renderer*> g_renderer_map;

    RendererImpl &get_renderer_impl(void);

    void update_lifecycle_render(LifecycleStage stage);
}
