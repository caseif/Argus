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

#include "argus/lowlevel/cabi/c_interop.h"
#include "argus/lowlevel/logging.hpp"

#include <string>
#include <vector>

typedef std::vector<std::string> StringArrayImpl;

extern "C" {

size_t string_array_get_count(StringArrayConst sa) {
    return reinterpret_cast<const StringArrayImpl *>(sa)->size();
}

const char *string_array_get_element(StringArrayConst sa, size_t index) {
    const auto &vec = *reinterpret_cast<const StringArrayImpl *>(sa);
    if (index >= vec.size()) {
        argus::Logger::default_logger()
                .fatal("Attempt to access invalid vector index %llu (vector size = %llu)", index, vec.size());
    }
    return vec[index].c_str();
}

void string_array_free(StringArray sa) {
    delete reinterpret_cast<StringArrayImpl *>(sa);
}

}
