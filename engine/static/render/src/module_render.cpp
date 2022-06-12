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

#include "argus/lowlevel/logging.hpp"

#include "argus/core/engine_config.hpp"
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"

#include "argus/render/defines.hpp"
#include "argus/render/common/canvas.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/common/backend.hpp"
#include "internal/render/loader/material_loader.hpp"
#include "internal/render/loader/texture_loader.hpp"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#define RENDER_BACKEND_MODULE_PREFIX "render_"

namespace argus {
    // forward declarations
    class Window;

    #if defined(__linux__) || defined(__ANDROID__)
    static const std::vector<std::string> g_default_backends = { "opengl", "opengl_es" };
    #elif defined(_WIN32) || defined(__MINGW__)
    static const std::vector<std::string> g_default_backends = { "opengl", "opengl_es" };
    #else
    static const std::vector<std::string> g_default_backends = { "opengl" };
    #endif

    bool g_render_module_initialized = false;

    static bool _try_backends(const std::vector<std::string> &backends, std::vector<std::string> attempted_backends) {
        for (auto backend : backends) {
            auto activate_fn = get_render_backend_activate_fn(backend);
            if (activate_fn == nullptr) {
                _ARGUS_INFO("Skipping unknown graphics backend \"%s\"", backend.c_str());
                attempted_backends.push_back(backend);
                continue;
            }

            if (!(*activate_fn)()) {
                _ARGUS_INFO("Unable to select graphics backend \"%s\"", backend.c_str());
                attempted_backends.push_back(backend);
                continue;
            }

            _ARGUS_INFO("Successfully activated graphics backend \"%s\"", backend.c_str());

            set_active_render_backend(backend);

            return true;
        }

        return false;
    }

    static void _activate_backend() {
        auto &backends = get_preferred_render_backends();

        std::vector<std::string> attempted_backends;

        if (_try_backends(backends, attempted_backends)) {
            return;
        }

        _ARGUS_WARN("Failed to select graphics backend from preference list, falling back to platform default");

        if (_try_backends(g_default_backends, attempted_backends)) {
            return;
        }

        _ARGUS_FATAL("Failed to select graphics backend");

        return;
    }

    static void _load_backend_modules(void) {
        _ARGUS_DEBUG("Loading graphics backend modules");
        size_t count = 0;
        for (auto &module : get_present_dynamic_modules()) {
            auto module_id = module.first;
            printf("%s\n", module_id.c_str());
            if (module_id.rfind(RENDER_BACKEND_MODULE_PREFIX, 0) == 0) {
                //TODO: fail gracefully
                enable_dynamic_module(module_id);
                count++;
            }
        }
        _ARGUS_DEBUG("Loaded %lu graphics backend modules", count);
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
            case LifecycleStage::PostDeinit: {
                unregister_backend_activate_fns();
                break;
            }
            default:
                break;
        }
    }
}
