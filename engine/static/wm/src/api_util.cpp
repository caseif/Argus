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

#include "argus/lowlevel/enum_ops.hpp"

#include "argus/wm/api_util.hpp"
#include "argus/wm/window.hpp"
#include "internal/wm/pimpl/window.hpp"

#include "SDL_vulkan.h"

namespace argus {
    int gl_load_library(void) {
        return SDL_GL_LoadLibrary(nullptr);
    }

    void gl_unload_library(void) {
        SDL_GL_UnloadLibrary();
    }

    GLContext gl_create_context(Window &window, GLContextFlags flags) {
        using namespace argus::enum_ops;

        auto profile_bits = flags & GLContextFlags::ProfileMask;
        if ((profile_bits & (int(profile_bits) - 1)) != 0) {
            //throw std::invalid_argument("Only one GL profile flag may be set during context creation aa");
        }

        if ((profile_bits & GLContextFlags::ProfileCore) != 0) {
            // need to request at least GL 3.2 to get a core profile
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        } else if ((profile_bits & GLContextFlags::ProfileES) != 0) {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        } else if ((profile_bits & GLContextFlags::ProfileCompat) != 0) {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        }

        SDL_GLcontextFlag context_flags = SDL_GLcontextFlag(0);
        if ((flags | GLContextFlags::DebugContext) != 0) {
            context_flags = SDL_GLcontextFlag(context_flags | SDL_GL_CONTEXT_DEBUG_FLAG);
        }
        // SDL doesn't support single-buffering, not sure why this is even a flag
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        auto ctx = SDL_GL_CreateContext(window.pimpl->handle);
        return ctx;
    }

    void gl_destroy_context(GLContext context) {
        SDL_GL_DeleteContext(context);
    }

    bool gl_is_context_current(GLContext context) {
        return SDL_GL_GetCurrentContext() == context;
    }

    int gl_make_context_current(Window &window, GLContext context) {
        return SDL_GL_MakeCurrent(window.pimpl->handle, context);
    }

    void *gl_load_proc(const char *name) {
        return SDL_GL_GetProcAddress(name);
    }

    void gl_swap_interval(int interval) {
        //SDL_GL_SetSwapInterval(interval);
        UNUSED(interval);
    }

    void gl_swap_buffers(Window &window) {
        SDL_GL_SwapWindow(window.pimpl->handle);
    }

    bool vk_is_supported(void) {
        return true;
    }

    int vk_create_surface(Window &window, void *instance, void **out_surface) {
        return SDL_Vulkan_CreateSurface(window.pimpl->handle, VkInstance(instance),
                reinterpret_cast<VkSurfaceKHR *>(out_surface));
    }

    int vk_get_instance_extensions(Window &window, unsigned int *out_count, const char **out_names) {
        return SDL_Vulkan_GetInstanceExtensions(window.pimpl->handle, out_count, out_names);
    }
}
