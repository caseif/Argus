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

// module core
#include "argus/core/engine_config.hpp"
#include "internal/core/engine_config.hpp"
#include "internal/core/module.hpp"

#include <algorithm>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

namespace argus {

    EngineConfig g_engine_config;

    EngineConfig get_engine_config() {
        return g_engine_config;
    }

    std::vector<RenderBackend> get_available_render_backends(void) {
        std::vector<RenderBackend> backends;

        auto modules = get_present_dynamic_modules();

        if (modules.find(std::string(RENDER_MODULE_OPENGL)) != modules.cend()) {
            backends.insert(backends.begin(), RenderBackend::OpenGL);
        }

        if (modules.find(std::string(RENDER_MODULE_OPENGLES)) != modules.cend()) {
            backends.insert(backends.begin(), RenderBackend::OpenGLES);
        }

        if (modules.find(std::string(RENDER_MODULE_VULKAN)) != modules.cend()) {
            backends.insert(backends.begin(), RenderBackend::Vulkan);
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

    void set_screen_space(const float left, const float right, const float bottom, const float top) {
        g_engine_config.screen_space = ScreenSpace(left, right, bottom, top);
    }
}