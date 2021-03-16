/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <string>
#include <vector>

namespace argus {
    struct pimpl_ResourceLoader {
        /**
         * \brief The ID of the type handled by this loader.
         */
        const std::string type_id;
        /**
         * \brief The file extensions this loader can handle.
         */
        const std::vector<std::string> extensions;

        /**
         * \brief The dependencies of the Resource last loaded.
         */
        std::vector<std::string> last_dependencies;

        pimpl_ResourceLoader(const std::string type_id, const std::vector<std::string> extensions):
            type_id(type_id),
            extensions(extensions) {
        }

        pimpl_ResourceLoader(const pimpl_ResourceLoader&) = delete;

        pimpl_ResourceLoader(pimpl_ResourceLoader&&) = delete;
    };
}
