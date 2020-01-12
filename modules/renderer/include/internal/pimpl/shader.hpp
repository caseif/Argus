/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/renderer.hpp"

#include <initializer_list>
#include <string>

namespace argus {
    struct pimpl_Shader {
        /**
         * \brief The type of this shader as a magic value.
         */
        const unsigned int type;
        /**
         * \brief The source code of this shader.
         */
        const std::string src;
        /**
         * \brief The name of this shader's entry point.
         */
        const std::string entry_point;
        /**
         * \brief The priority of this shader.
         *
         * Higher priority shaders will be processed before lower priority
         * ones within their respective stage.
         */
        const int priority;
        /**
         * \brief The uniforms defined by this shader.
         */
        const std::vector<std::string> uniform_ids;

        pimpl_Shader(const unsigned int type, const std::string &src, const std::string &entry_point,
            const int priority, const std::initializer_list<std::string> &uniform_ids):
                type(type),
                src(src),
                entry_point(entry_point),
                priority(priority),
                uniform_ids(uniform_ids) {
        }
    };
}