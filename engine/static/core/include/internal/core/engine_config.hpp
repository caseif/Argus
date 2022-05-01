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

#pragma once

#include "argus/lowlevel/math.hpp"

#include "argus/core/engine_config.hpp"

#include <string>
#include <vector>

namespace argus {
    struct EngineConfig {
        unsigned int target_tickrate{0};
        unsigned int target_framerate{0};
        std::vector<std::string> load_modules;
        std::vector<RenderBackend> render_backends;
        ScreenSpaceScaleMode screen_space_scale_mode;
    };

    EngineConfig get_engine_config();

    RenderBackend get_selected_render_backend(void);

    void set_selected_render_backend(RenderBackend backend);
}
