/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"

// module core
#include "argus/core/engine.hpp"
#include "argus/core/engine_config.hpp"
#include "internal/core/engine_config.hpp"
#include "internal/core/module.hpp"

#include <initializer_list>
#include <string>
#include <vector>

namespace argus {

    EngineConfig g_engine_config;

    EngineConfig get_engine_config() {
        return g_engine_config;
    }

    std::vector<RenderBackend> get_available_render_backends(void) {
        std::vector<RenderBackend> backends;

        auto modules = get_present_external_modules();
        
        if (modules.find(std::string(RENDER_MODULE_OPENGL)) != modules.cend()) {
            backends.insert(backends.begin(), RenderBackend::OPENGL);
        }

        if (modules.find(std::string(RENDER_MODULE_OPENGLES)) != modules.cend()) {
            backends.insert(backends.begin(), RenderBackend::OPENGLES);
        }

        if (modules.find(std::string(RENDER_MODULE_VULKAN)) != modules.cend()) {
            backends.insert(backends.begin(), RenderBackend::VULKAN);
        }

        return backends;
    }

    void set_target_tickrate(const unsigned int target_tickrate) {
        g_engine_config.target_tickrate = target_tickrate;
    }

    void set_target_framerate(const unsigned int target_framerate) {
        g_engine_config.target_framerate = target_framerate;
    }

    void set_load_modules(const std::initializer_list<std::string> &module_list) {
        g_engine_config.load_modules.insert(g_engine_config.load_modules.begin(), module_list);
    }

    void set_render_backend(const std::initializer_list<RenderBackend> backends) {
        g_engine_config.render_backends = std::vector<RenderBackend>(backends);
    }

    void set_render_backend(const RenderBackend backend) {
        set_render_backend({ backend });
    }

    void set_screen_space(ScreenSpace screen_space) {
        g_engine_config.screen_space = screen_space;
    }

    void set_screen_space(float left, float right, float bottom, float top) {
        g_engine_config.screen_space = ScreenSpace(left, right, bottom, top);
    }
}
