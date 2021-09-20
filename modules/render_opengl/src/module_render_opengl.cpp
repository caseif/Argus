/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"

// module core
#include "argus/core/module.hpp"
#include "internal/core/dyn_invoke.hpp"
#include "internal/core/engine_config.hpp"

// module resman
#include "argus/resman/resource_manager.hpp"

// module render
#include "internal/render/defines.hpp"

// module render_opengl
#include "internal/render_opengl/glfw_include.hpp"
#include "internal/render_opengl/module_render_opengl.hpp"
#include "internal/render_opengl/resources_render_opengl.arp.h"
#include "internal/render_opengl/loader/shader_loader.hpp"
#include "internal/render_opengl/renderer/gl_renderer_base.hpp"

#include <string>

#include <cstring>

namespace argus {
class RendererImpl;

    Matrix4 g_view_matrix;

    static RendererImpl *_create_opengl_backend() {
        return new GLRenderer();
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

    void update_lifecycle_render_opengl(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PreInit: {
                register_module_fn(FN_CREATE_OPENGL_BACKEND, reinterpret_cast<void*>(_create_opengl_backend));
                break;
            }
            case LifecycleStage::Init: {
                ResourceManager::get_global_resource_manager().register_loader(*new ShaderLoader());                

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
                ResourceManager::get_global_resource_manager().add_memory_package(RESOURCES_RENDER_OPENGL_ARP_SRC,
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
