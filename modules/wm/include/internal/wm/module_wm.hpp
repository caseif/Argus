/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "argus/core/module.hpp"

#include <map>

#include <cstddef>

struct GLFWwindow;

namespace argus {
    // forward declarations
    class Window;

    extern bool g_wm_module_initialized;

    extern std::map<GLFWwindow*, Window*> g_window_map;
    extern size_t g_window_count;

    void init_module_wm(void);

    void update_lifecycle_wm(LifecycleStage stage);
}
