/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include <string>
#include <vector>

namespace argus {
    struct pimpl_ResourceLoader {
        /**
         * \brief The media types handled by this loader.
         */
        const std::vector<std::string> media_types;

        /**
         * \brief The dependencies of the Resource last loaded.
         */
        //TODO: this is super non-threadsafe and needs to be reworked
        std::vector<std::string> last_dependencies;

        pimpl_ResourceLoader(std::initializer_list<std::string> &media_types):
            media_types(media_types) {
        }

        pimpl_ResourceLoader(const pimpl_ResourceLoader&) = delete;

        pimpl_ResourceLoader(pimpl_ResourceLoader&&) = delete;
    };
}
