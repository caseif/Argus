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

#include "internal/render_opengl/state/linked_program.hpp"

#include <optional>

namespace argus {
    std::optional<uniform_location_t> LinkedProgram::get_uniform_loc(const std::string &name) {
        auto it = reflection_info.uniform_variable_locations.find(name);
        return it != reflection_info.uniform_variable_locations.end()
            ? std::make_optional(it->second)
            : std::nullopt;
    }

    void LinkedProgram::get_uniform_loc_and_then(const std::string &name, std::function<void(uniform_location_t)> fn) {
        auto loc = get_uniform_loc(name);
        if (loc.has_value()) {
            fn(loc.value());
        }
    }
}
