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

#include "argus/wm/window.hpp"

namespace argus {
    typedef void *GLContext;

    GLContext gl_create_context(Window &window);

    void gl_destroy_context(GLContext context);

    bool gl_is_context_current(GLContext context);

    void gl_make_context_current(Window &window, GLContext context);

    void *gl_load_proc(const char *name);

    void gl_swap_interval(int interval);

    void gl_swap_buffers(Window &window);

    bool vk_is_supported(void);

    void vk_create_surface(Window &window, void *instance, void **out_surface);

    void vk_get_instance_extensions(Window &window, unsigned int *out_count, const char **out_names);
}
