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

#include <functional>
#include <map>
#include <optional>
#include <string>

namespace argus {
    struct GlobalUniformLocs {
        std::optional<uint32_t> uni_view_matrix;
        std::optional<uint32_t> uni_time;
    };

    struct LinkedProgram {
        uint32_t handle;
        ShaderReflectionInfo reflection_info;

        LinkedProgram(uint32_t handle, ShaderReflectionInfo reflection_info):
            handle(handle),
            reflection_info(reflection_info) {
        }

        bool has_attr(const std::string &name);

        std::optional<uint32_t> get_attr_loc(const std::string &name);

        void get_attr_loc_and_then(const std::string &name, std::function<void(uint32_t)> fn);

        bool has_uniform(const std::string &name);

        std::optional<uint32_t> get_uniform_loc(const std::string &name);

        void get_uniform_loc_and_then(const std::string &name, std::function<void(uint32_t)> fn);
    };
}