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

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/engine_config.hpp"
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"
#include "internal/core/dyn_invoke.hpp"
#include "internal/core/engine_config.hpp"
#include "internal/core/module.hpp"

// module resman
#include "argus/resman/resource_manager.hpp"

// module wm
#include "internal/wm/window.hpp"

// module render
#include "argus/render/common/renderer.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/renderer.hpp"
#include "internal/render/loader/material_loader.hpp"
#include "internal/render/loader/texture_loader.hpp"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Window;

    bool g_render_module_initialized = false;

    std::map<const Window*, Renderer*> g_renderer_map;

    static void _activate_backend() {
        auto backends = get_engine_config().render_backends;

        for (auto backend : backends) {
            switch (backend) {
                case RenderBackend::OpenGL: {
                    call_module_fn<void>(std::string(FN_ACTIVATE_OPENGL_BACKEND));
                    _ARGUS_INFO("Selecting OpenGL as graphics backend\n");
                    return;
                }
                case RenderBackend::OpenGLES:
                    _ARGUS_INFO("Graphics backend OpenGL ES is not yet supported\n");
                    break;
                case RenderBackend::Vulkan:
                    _ARGUS_INFO("Graphics backend Vulkan is not yet supported\n");
                    break;
                default:
                    _ARGUS_WARN("Skipping unrecognized graphics backend index %d\n", static_cast<int>(backend));
                    break;
            }
            _ARGUS_INFO("Current graphics backend cannot be selected, continuing to next\n");
        }

        //TODO: select fallback based on platform
        _ARGUS_WARN("Failed to select graphics backend from preference list, defaulting to OpenGL\n");

        call_module_fn<void>(std::string(FN_ACTIVATE_OPENGL_BACKEND));
        return;
    }

    static void _window_construct_callback(Window &window) {
        auto *renderer = new Renderer(window);
        g_renderer_map.insert({&window, renderer});
    }

    static void _load_backend_modules(void) {
        //TODO: fail gracefully
        enable_dynamic_module(MODULE_RENDER_OPENGL);
    }

    void update_lifecycle_render(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Load: {
                _load_backend_modules();
                break;
            }
            case LifecycleStage::Init: {
                _activate_backend();

                set_window_construct_callback(_window_construct_callback);

                register_event_handler(ArgusEventType::Window, renderer_window_event_callback, TargetThread::Render);

                ResourceManager::instance().register_loader(*new MaterialLoader());
                ResourceManager::instance().register_loader(*new PngTextureLoader());

                g_render_module_initialized = true;

                break;
            }
            default:
                break;
        }
    }
}
