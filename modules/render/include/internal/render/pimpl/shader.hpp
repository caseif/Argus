/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/shader.hpp"

#include <initializer_list>
#include <string>
#include <vector>

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
         * \brief The order of this shader.
         *
         * Shaders with a lower order value will be processed first.
         */
        const int order;
        /**
         * \brief The uniforms defined by this shader.
         */
        const std::vector<std::string> uniform_ids;

        pimpl_Shader(const unsigned int type, const std::string &src, const std::string &entry_point,
            const int order, const std::initializer_list<std::string> &uniform_ids):
                type(type),
                src(src),
                entry_point(entry_point),
                order(order),
                uniform_ids(uniform_ids) {
        }
    };
}
