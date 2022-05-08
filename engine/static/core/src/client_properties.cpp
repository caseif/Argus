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

#include "argus/core/client_properties.hpp"
#include "internal/core/client_properties.hpp"

#include <string>

namespace argus {
    static ClientProperties g_client_properties;

    void set_client_id(const std::string &id) {
        g_client_properties.id = id;
    }

    void set_client_name(const std::string &name) {
        g_client_properties.name = name;
    }

    void set_client_version(const std::string &version) {
        g_client_properties.version = version;
    }

    ClientProperties &get_client_properties(void) {
        return g_client_properties;
    }
}
