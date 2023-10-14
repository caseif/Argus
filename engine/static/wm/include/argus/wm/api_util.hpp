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

    enum class GLContextFlags {
        None              = 0x0,
        ProfileCore       = 0x1,
        ProfileES         = 0x2,
        ProfileCompat     = 0x4,
        DebugContext      = 0x100,
        ProfileMask = int(GLContextFlags::ProfileCore)
                |     int(GLContextFlags::ProfileES)
                |     int(GLContextFlags::ProfileCompat),
    };

    inline GLContextFlags operator&(GLContextFlags a, GLContextFlags b) {
        using U = std::underlying_type_t<GLContextFlags>;
        return GLContextFlags(U(a) & U(b));
    }

    inline GLContextFlags operator|(GLContextFlags a, GLContextFlags b) {
        using U = std::underlying_type_t<GLContextFlags>;
        return GLContextFlags(U(a) | U(b));
    }

    int gl_load_library(void);

    void gl_unload_library(void);

    GLContext gl_create_context(Window &window, int version_major, int version_minor, GLContextFlags flags);

    void gl_destroy_context(GLContext context);

    bool gl_is_context_current(GLContext context);

    int gl_make_context_current(Window &window, GLContext context);

    void *gl_load_proc(const char *name);

    void gl_swap_interval(int interval);

    void gl_swap_buffers(Window &window);

    bool vk_is_supported(void);

    int vk_create_surface(Window &window, void *instance, void **out_surface);

    int vk_get_instance_extensions(Window &window, unsigned int *out_count, const char **out_names);
}
