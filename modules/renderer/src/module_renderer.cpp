/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core.hpp"

// module resman
#include "argus/resource_manager.hpp"

// module renderer
#include "argus/renderer/window.hpp"
#include "argus/renderer/window_event.hpp"
#include "internal/renderer/defines.hpp"
#include "internal/renderer/glext.hpp"
#include "internal/renderer/texture_loader.hpp"

#include <GLFW/glfw3.h>

#include <iterator>
#include <map>
#include <string>
#include <utility>

#include <cstddef>
#include <cstdint>

namespace argus {

    bool g_renderer_initialized = false;

    // maps SDL window IDs to Window instance pointers
    std::map<uint32_t, Window*> g_window_map;
    size_t g_window_count = 0;

    static void _init_opengl(void) {
        /*#ifdef USE_GLES
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        #else
        context_flags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        #endif

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);*/
    }

    static void _clean_up(void) {
        // use a copy since Window::destroy modifies the global list
        auto windows_copy = g_window_map;
        // doing this in reverse ensures that child windows are destroyed before their parents
        for (auto it = windows_copy.rbegin();
                it != windows_copy.rend(); it++) {
            it->second->destroy();
        }

        glfwTerminate();

        return;
    }

    static void poll_events(const TimeDelta delta) {
        glfwPollEvents();
    }

    void _update_lifecycle_renderer(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::INIT: {
                glfwInit();
                _init_opengl();

                register_render_callback(poll_events);

                ResourceManager::get_global_resource_manager()
                    .register_loader(RESOURCE_TYPE_TEXTURE_PNG, new PngTextureLoader());

                g_renderer_initialized = true;

                break;
            }
            case LifecycleStage::DEINIT:
                _clean_up();
                break;
            default:
                break;
        }
    }

    void init_module_renderer(void) {
        register_module({MODULE_RENDERER, 3, {"core", "resman"}, _update_lifecycle_renderer});
    }

}
