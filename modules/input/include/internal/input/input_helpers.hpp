/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <GLFW/glfw3.h>

namespace argus {

    void init_window_input(GLFWwindow *handle);

    void init_keyboard(GLFWwindow *handle);

}