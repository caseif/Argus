/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
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
         * \brief The stage this shader is to be run at.
         */
        const ShaderStage stage;
        /**
         * \brief The source code for this shader.
         */
        const char *const src;
        /**
         * \brief The length of the shader's source code in bytes.
         */
        const size_t src_len;

        pimpl_Shader(const ShaderStage stage, const char *const src, const size_t src_len):
                stage(stage),
                src(src),
                src_len(src_len) {
        }

        pimpl_Shader(const pimpl_Shader&) = default;

        pimpl_Shader(pimpl_Shader&&) = delete;
    };
}
