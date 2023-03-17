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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/math.hpp"

#include "argus/core/module.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/backend.hpp"

#include "internal/render_opengl_legacy/defines.hpp"
#include "internal/render_opengl_legacy/module_render_opengl_legacy.hpp"
#include "internal/render_opengl_legacy/resources.h"
#include "internal/render_opengl_legacy/loader/shader_loader.hpp"
#include "internal/render_opengl_legacy/renderer/gl_renderer.hpp"

#include "aglet/aglet.h"
#include "GLFW/glfw3.h"

#include <string>

#include <cstring>

namespace argus {
    static bool g_backend_active = false;
    static std::map<const Window *, GLRenderer *> g_renderer_map;

    static bool _test_opengl_support() {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        // we _could_ set the minimum GL version as a window hint here, but the
        // error message would then be totally useless to end users
        auto *window = glfwCreateWindow(1, 1, "", nullptr, nullptr);

        if (window == nullptr) {
            Logger::default_logger().warn("Failed to detect OpenGL capabilities (GLFW failed to open window)");
            return false;
        }

        glfwMakeContextCurrent(window);

        auto cap_rc = agletLoadCapabilities(reinterpret_cast<AgletLoadProc>(glfwGetProcAddress));

        switch (cap_rc) {
            case AGLET_ERROR_NONE:
                break;
            case AGLET_ERROR_UNSPECIFIED:
                Logger::default_logger().warn("Aglet failed to load OpenGL bindings (unspecified error)");
                return false;
            case AGLET_ERROR_PROC_LOAD:
                Logger::default_logger().warn("Aglet failed to load prerequisite OpenGL procs");
                return false;
            case AGLET_ERROR_GL_ERROR:
                Logger::default_logger().warn("Aglet failed to load OpenGL bindings (OpenGL error)");
                return false;
            case AGLET_ERROR_MINIMUM_VERSION:
                Logger::default_logger().warn("Argus requires support for OpenGL 3.3 or higher");
                return false;
            case AGLET_ERROR_MISSING_EXTENSION:
                Logger::default_logger().warn("Required OpenGL extensions are not available");
                return false;
        }

        glfwDestroyWindow(window);

        return true;
    }

    static bool _activate_opengl_backend() {
        if (!_test_opengl_support()) {
            return false;
        }

        g_backend_active = true;
        return true;
    }

    static void _window_event_callback(const WindowEvent &event, void *user_data) {
        UNUSED(user_data);
        auto &window = event.window;

        switch (event.subtype) {
            case WindowEventType::Create: {
                auto *renderer = new GLRenderer(window);
                g_renderer_map.insert({&window, renderer});
                break;
            }
            case WindowEventType::Update: {
                if (!window.is_ready()) {
                    return;
                }

                auto it = g_renderer_map.find(&window);
                assert(it != g_renderer_map.end());

                it->second->render(event.delta);
                break;
            }
            case WindowEventType::Resize: {
                if (!window.is_ready()) {
                    return;
                }

                auto it = g_renderer_map.find(&window);
                assert(it != g_renderer_map.end());

                it->second->notify_window_resize(event.resolution);
                break;
            }
            case WindowEventType::RequestClose: {
                auto it = g_renderer_map.find(&window);
                assert(it != g_renderer_map.end());

                delete it->second;
                break;
            }
            default: {
                break;
            }
        }
    }

    void update_lifecycle_render_opengl_legacy(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PreInit: {
                register_render_backend(BACKEND_ID, _activate_opengl_backend);
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
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
                #ifdef _ARGUS_DEBUG_MODE
                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
                #endif

                break;
            }
            case LifecycleStage::PostInit: {
                if (!g_backend_active) {
                    return;
                }

                ResourceManager::instance().add_memory_package(RESOURCES_RENDER_OPENGL_LEGACY_ARP_SRC,
                        RESOURCES_RENDER_OPENGL_LEGACY_ARP_LEN);
                break;
            }
            default: {
                break;
            }
        }
    }

    REGISTER_ARGUS_MODULE("render_opengl_legacy", update_lifecycle_render_opengl_legacy, { "render" });
}