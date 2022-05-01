/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#include "internal/lowlevel/logging.hpp"

#include "argus/core/engine_config.hpp"
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"
#include "internal/core/dyn_invoke.hpp"
#include "internal/core/engine_config.hpp"
#include "internal/core/module.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"
#include "internal/wm/window.hpp"

#include "argus/render/common/canvas.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/module_render.hpp"
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

    static void _activate_backend() {
        auto backends = get_engine_config().render_backends;

        std::map<RenderBackend, bool> attempted_backends;

        //TODO: test backend before selecting
        for (auto backend : backends) {
            switch (backend) {
                case RenderBackend::OpenGL: {
                    attempted_backends[backend] = true;

                    if (!call_module_fn<bool>(std::string(FN_ACTIVATE_OPENGL_BACKEND))) {
                        _ARGUS_WARN("Unable to select OpenGL as graphics backend");
                        continue;
                    }
                    
                    set_selected_render_backend(backend);
                    _ARGUS_INFO("Selecting OpenGL as graphics backend");
                    
                    return;
                }
                case RenderBackend::OpenGLES:
                    attempted_backends[backend] = true;

                    if (!call_module_fn<bool>(std::string(FN_ACTIVATE_OPENGLES_BACKEND))) {
                        _ARGUS_WARN("Unable to select OpenGL as graphics backend");
                        continue;
                    }
                    
                    set_selected_render_backend(backend);
                    _ARGUS_INFO("Selecting OpenGL ES as graphics backend");
                    
                    return;
                case RenderBackend::Vulkan:
                    attempted_backends[backend] = true;

                    if (!call_module_fn<bool>(std::string(FN_ACTIVATE_VULKAN_BACKEND))) {
                        _ARGUS_WARN("Unable to select Vulkan as graphics backend");
                        continue;
                    }
                    
                    set_selected_render_backend(backend);
                    _ARGUS_INFO("Selecting Vulkan as graphics backend");

                    return;
                default:
                    _ARGUS_WARN("Skipping unrecognized graphics backend index %d", static_cast<int>(backend));
                    break;
            }
            _ARGUS_INFO("Current graphics backend cannot be selected, continuing to next");
        }

        //TODO: select fallback based on platform
        _ARGUS_WARN("Failed to select graphics backend from preference list, falling back to platform default");

        RenderBackend selected_backend;

        #if defined(__linux__) && !defined(__ANDROID__)
        _ARGUS_INFO("Detected Linux, using OpenGL as platform default");

        if (attempted_backends.find(RenderBackend::OpenGL) != attempted_backends.end()) {
            _ARGUS_FATAL("Already attempted to use OpenGL, unable to use platform default");
        }

        selected_backend = RenderBackend::OpenGL;
        call_module_fn<void>(std::string(FN_ACTIVATE_OPENGL_BACKEND));
        #else
        _ARGUS_INFO("Failed to select default for current system, using OpenGL as meta-default");
        selected_backend = RenderBackend::OpenGL;
        call_module_fn<void>(std::string(FN_ACTIVATE_OPENGL_BACKEND));
        #endif

        set_selected_render_backend(selected_backend);

        return;
    }

    static void _load_backend_modules(void) {
        //TODO: fail gracefully
        enable_dynamic_module(MODULE_RENDER_OPENGL);
        enable_dynamic_module(MODULE_RENDER_OPENGLES);
        enable_dynamic_module(MODULE_RENDER_VULKAN);
    }

    static Canvas &_construct_canvas(Window &window) {
        return *new Canvas(window);
    }

    static void _destroy_canvas(Canvas &canvas) {
        delete &canvas;
    }

    void update_lifecycle_render(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Load: {
                _ARGUS_DEBUG("Loading backend modules for rendering");

                _load_backend_modules();

                break;
            }
            case LifecycleStage::Init: {
                _ARGUS_DEBUG("Activating render backend module");

                _activate_backend();

                Window::set_canvas_ctor_and_dtor(_construct_canvas, _destroy_canvas);

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
