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

#include "argus/lowlevel/math.hpp"

#include "argus/core/engine_config.hpp"
#include "internal/core/engine_config.hpp"
#include "internal/core/module.hpp"

#include <algorithm>
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace argus {

    EngineConfig g_engine_config;

    std::string g_selected_render_backend;

    EngineConfig get_engine_config() {
        return g_engine_config;
    }

    void set_target_tickrate(const unsigned int target_tickrate) {
        g_engine_config.target_tickrate = target_tickrate;
    }

    void set_target_framerate(const unsigned int target_framerate) {
        g_engine_config.target_framerate = target_framerate;
    }

    void set_load_modules(const std::initializer_list<std::string> &module_list) {
        g_engine_config.load_modules = std::vector<std::string>(module_list);
    }

    void set_load_modules(const std::vector<std::string> &module_list) {
        g_engine_config.load_modules = module_list;
    }

    void set_render_backends(const std::initializer_list<std::string> &backends) {
        g_engine_config.render_backends = std::vector<std::string>(backends);
    }

    void set_render_backends(const std::vector<std::string> &backends) {
        g_engine_config.render_backends = backends;
    }

    void set_render_backend(const std::string backend) {
        set_render_backends({ backend });
    }

    void set_screen_space_scale_mode(ScreenSpaceScaleMode scale_mode) {
        g_engine_config.screen_space_scale_mode = scale_mode;
    }

    const std::string &get_selected_render_backend(void) {
        return g_selected_render_backend;
    }

    void set_selected_render_backend(const std::string &backend) {
        g_selected_render_backend = backend;
    }
}
