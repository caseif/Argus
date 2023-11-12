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
#include "argus/lowlevel/math.hpp"

#include "argus/core/module.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/backend.hpp"

#include "internal/render_opengles/defines.hpp"
#include "internal/render_opengles/module_render_opengl.hpp"
#include "internal/render_opengles/resources.h"
#include "internal/render_opengles/loader/shader_loader.hpp"
#include "internal/render_opengles/renderer/gles_renderer.hpp"

#include <string>

namespace argus {
    static bool g_backend_active = false;
    static std::map<const Window *, GLESRenderer *> g_renderer_map;

    static bool _test_opengles_support() {
        auto &window = Window::create("", nullptr);
        window.update({});
        GLContext gl_context;
        if ((gl_context = gl_create_context(window, 3, 0, GLContextFlags::ProfileES)) == nullptr) {
            Logger::default_logger().warn("Failed to create GL ES context");
            return false;
        }

        auto rc = gl_make_context_current(window, gl_context);
        if (rc != 0) {
            Logger::default_logger().warn("Failed to make GL ES context current (%d)", rc);
        }

        auto cap_rc = agletLoadCapabilities(reinterpret_cast<AgletLoadProc>(gl_load_proc));

        switch (cap_rc) {
            case AGLET_ERROR_NONE:
                break;
            case AGLET_ERROR_UNSPECIFIED:
                Logger::default_logger().warn("Aglet failed to load OpenGL ES bindings (unspecified error)");
            return false;
            case AGLET_ERROR_PROC_LOAD:
                Logger::default_logger().warn("Aglet failed to load prerequisite OpenGL ES procs");
            return false;
            case AGLET_ERROR_GL_ERROR:
                Logger::default_logger().warn("Aglet failed to load OpenGL ES bindings (OpenGL ES error)");
            return false;
            case AGLET_ERROR_MINIMUM_VERSION:
                Logger::default_logger().warn("Argus requires support for OpenGL ES 3.0 or higher");
            return false;
            case AGLET_ERROR_MISSING_EXTENSION:
                Logger::default_logger().warn("Required OpenGL ES extensions are not available");
            return false;
        }

        window.request_close();

        return true;
    }

    static bool _activate_opengles_backend() {
        set_window_creation_flags(WindowCreationFlags::OpenGL);

        if (gl_load_library() != 0) {
            Logger::default_logger().warn("Failed to load OpenGL ES library");
            set_window_creation_flags(WindowCreationFlags::None);
            return false;
        }

        if (!_test_opengles_support()) {
            gl_unload_library();
            set_window_creation_flags(WindowCreationFlags::None);
            return false;
        }

        g_backend_active = true;
        return true;
    }

    static void _window_event_callback(const WindowEvent &event, void *user_data) {
        UNUSED(user_data);
        Window &window = event.window;

        switch (event.subtype) {
            case WindowEventType::Create: {
                auto *renderer = new GLESRenderer(window);
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

    void update_lifecycle_render_opengles(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PreInit: {
                register_render_backend(BACKEND_ID, _activate_opengles_backend);
                break;
            }
            case LifecycleStage::Init: {
                if (!g_backend_active) {
                    return;
                }

                ResourceManager::instance().register_loader(*new ShaderLoader());

                register_event_handler<WindowEvent>(_window_event_callback, TargetThread::Render);

                break;
            }
            case LifecycleStage::PostInit: {
                if (!g_backend_active) {
                    return;
                }

                ResourceManager::instance().add_memory_package(RESOURCES_RENDER_OPENGLES_ARP_SRC,
                        RESOURCES_RENDER_OPENGLES_ARP_LEN);
                break;
            }
            case LifecycleStage::PostDeinit: {
                gl_unload_library();

                break;
            }
            default: {
                break;
            }
        }
    }

    REGISTER_ARGUS_MODULE("render_opengles", update_lifecycle_render_opengles, { "render" })
}
