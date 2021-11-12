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
#include "argus/lowlevel/math.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/module.hpp"
#include "internal/core/dyn_invoke.hpp"
#include "internal/core/engine_config.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"

// module resman
#include "argus/resman/resource_manager.hpp"

// module render
#include "internal/render/defines.hpp"

// module render_opengl
#include "internal/render_opengl/glfw_include.hpp"
#include "internal/render_opengl/module_render_opengl.hpp"
#include "internal/render_opengl/resources.h"
#include "internal/render_opengl/loader/shader_loader.hpp"
#include "internal/render_opengl/renderer/gl_renderer.hpp"

#include <string>

#include <cstring>

namespace argus {
    static bool g_backend_active = false;
    static std::map<const Window*, GLRenderer*> g_renderer_map;

    Matrix4 g_view_matrix;

    static void _activate_opengl_backend() {
        g_backend_active = true;
    }

    static void _setup_view_matrix() {
        auto screen_space = get_engine_config().screen_space;
        
        auto l = screen_space.left;
        auto r = screen_space.right;
        auto b = screen_space.bottom;
        auto t = screen_space.top;
        
        g_view_matrix = {
            {2 / (r - l), 0, 0, 0},
            {0, 2 / (t - b), 0, 0},
            {0, 0, 1, 0},
            {-(r + l) / (r - l), -(t + b) / (t - b), 0, 1}
        };
    }

    static void _window_event_callback(const ArgusEvent &event, void *user_data) {
        UNUSED(user_data);
        const WindowEvent &window_event = static_cast<const WindowEvent&>(event);

        const Window &window = window_event.window;

        switch (window_event.subtype) {
            case WindowEventType::Create: {
                auto *renderer = new GLRenderer(window);
                g_renderer_map.insert({ &window, renderer });
                break;
            }
            case WindowEventType::Update: {
                if (!window.is_ready()) {
                    return;
                }
                
                auto it = g_renderer_map.find(&window);
                _ARGUS_ASSERT(it != g_renderer_map.end(), "Received window update but no renderer was registered!");

                it->second->render(window_event.delta);
                break;
            }
            case WindowEventType::RequestClose: {
                auto it = g_renderer_map.find(&window);
                _ARGUS_ASSERT(it != g_renderer_map.end(), "Received window close request but no renderer was registered!");

                delete it->second;
                break;
            }
            default: {
                break;
            }
        }
    }

    void update_lifecycle_render_opengl(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PreInit: {
                register_module_fn(FN_ACTIVATE_OPENGL_BACKEND, reinterpret_cast<void*>(_activate_opengl_backend));
                break;
            }
            case LifecycleStage::Init: {
                if (!g_backend_active) {
                    return;
                }

                ResourceManager::instance().register_loader(*new ShaderLoader());

                register_event_handler<WindowEvent>(_window_event_callback, TargetThread::Render);

                glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
                glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
                #ifdef _ARGUS_DEBUG_MODE
                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
                #endif

                _setup_view_matrix();
                break;
            }
            case LifecycleStage::PostInit: {
                if (!g_backend_active) {
                    return;
                }

                ResourceManager::instance().add_memory_package(RESOURCES_RENDER_OPENGL_ARP_SRC,
                        RESOURCES_RENDER_OPENGL_ARP_LEN);
                break;
            }
            default: {
                break;
            }
        }
    }

    REGISTER_ARGUS_MODULE("render_opengl", update_lifecycle_render_opengl, { "render" });
}
