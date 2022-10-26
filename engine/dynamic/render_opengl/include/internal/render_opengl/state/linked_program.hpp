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

#include "argus/render/common/shader.hpp"
#include "internal/render_opengl/types.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>

namespace argus {
    struct GlobalUniformLocs {
        std::optional<uniform_location_t> uni_view_matrix;
        std::optional<uniform_location_t> uni_time;
    };

    struct LinkedProgram {
        program_handle_t handle;
        std::optional<attribute_location_t> attr_position_loc;
        std::optional<attribute_location_t> attr_normal_loc;
        std::optional<attribute_location_t> attr_color_loc;
        std::optional<attribute_location_t> attr_texcoord_loc;
        ShaderReflectionInfo reflection_info;

        LinkedProgram(program_handle_t handle, attribute_location_t attr_pos, attribute_location_t attr_norm,
                attribute_location_t attr_color, attribute_location_t attr_tc, ShaderReflectionInfo reflection_info):
            handle(handle),
            attr_position_loc(attr_pos != -1 ? std::optional(attr_pos) : std::nullopt),
            attr_normal_loc(attr_norm != -1 ? std::optional(attr_norm) : std::nullopt),
            attr_color_loc(attr_color != -1 ? std::optional(attr_color) : std::nullopt),
            attr_texcoord_loc(attr_tc != -1 ? std::optional(attr_tc) : std::nullopt),
            reflection_info(reflection_info) {
        }

        std::optional<uniform_location_t> get_uniform_loc(const std::string &name);

        void get_uniform_loc_and_then(const std::string &name, std::function<void(uniform_location_t)> fn);
    };
}