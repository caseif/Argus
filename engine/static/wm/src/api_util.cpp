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

#include "argus/wm/api_util.hpp"
#include "argus/wm/window.hpp"
#include "internal/wm/pimpl/window.hpp"

#include "SDL2/SDL_opengl.h"
#include "SDL2/SDL_vulkan.h"

namespace argus {
    GLContext gl_create_context(Window &window) {
        return SDL_GL_CreateContext(window.pimpl->handle);
    }

    void gl_destroy_context(GLContext context) {
        SDL_GL_DeleteContext(context);
    }

    bool gl_is_context_current(GLContext context) {
        return SDL_GL_GetCurrentContext() == context;
    }

    void gl_make_context_current(Window &window, GLContext context) {
        SDL_GL_MakeCurrent(window.pimpl->handle, context);
    }

    void *gl_load_proc(const char *name) {
        return SDL_GL_GetProcAddress(name);
    }

    void gl_swap_interval(int interval) {
        SDL_GL_SetSwapInterval(interval);
    }

    void gl_swap_buffers(Window &window) {
        SDL_GL_SwapWindow(window.pimpl->handle);
    }

    bool vk_is_supported(void) {
        return true;
    }

    void vk_create_surface(Window &window, void *instance, void **out_surface) {
        SDL_Vulkan_CreateSurface(window.pimpl->handle, VkInstance(instance),
                reinterpret_cast<VkSurfaceKHR *>(out_surface));
    }

    void vk_get_instance_extensions(Window &window, unsigned int *out_count, const char **out_names) {
        SDL_Vulkan_GetInstanceExtensions(window.pimpl->handle, out_count, out_names);
    }
}
