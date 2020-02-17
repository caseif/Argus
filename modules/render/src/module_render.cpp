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

// module render
#include "argus/render/window.hpp"
#include "argus/render/window_event.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/glext.hpp"
#include "internal/render/texture_loader.hpp"

#include <GLFW/glfw3.h>

#include <iterator>
#include <map>
#include <string>
#include <utility>

#include <cstddef>
#include <cstdint>

namespace argus {

    bool g_render_module_initialized = false;

    // maps GLFW window pointers to Window instance pointers
    std::map<GLFWwindow*, Window*> g_window_map;
    size_t g_window_count = 0;

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

    void _update_lifecycle_render(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::INIT: {
                glfwInit();

                register_render_callback(poll_events);

                ResourceManager::get_global_resource_manager()
                    .register_loader(RESOURCE_TYPE_TEXTURE_PNG, new PngTextureLoader());

                g_render_module_initialized = true;

                break;
            }
            case LifecycleStage::DEINIT:
                _clean_up();
                break;
            default:
                break;
        }
    }

    void init_module_render(void) {
        register_module({MODULE_RENDER, 3, {"core", "resman"}, _update_lifecycle_render});
    }

}
