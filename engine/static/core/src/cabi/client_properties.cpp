/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/core/cabi/client_properties.h"

#include "argus/core/client_properties.hpp"

const char *argus_get_client_id(void) {
    return argus::get_client_id().c_str();
}

void argus_set_client_id(const char *id) {
    argus::set_client_id(id);
}

const char *argus_get_client_name(void) {
    return argus::get_client_name().c_str();
}

void argus_set_client_name(const char *id) {
    argus::set_client_name(id);
}

const char *argus_get_client_version(void) {
    return argus::get_client_version().c_str();
}

void argus_set_client_version(const char *id) {
    argus::set_client_version(id);
}
