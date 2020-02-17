/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "argus/render/shader.hpp"
#include "internal/render/types.hpp"

#include <set>
#include <unordered_map>

namespace argus {
    struct pimpl_ShaderProgram {
        /**
         * \brief The set of Shaders encompassed by this program.
         */
        std::set<const Shader*, bool(*)(const Shader*, const Shader*)> shaders;
        /**
         * \brief A complete list of uniforms defined by this
         *        program's Shaders.
         */
        std::unordered_map<std::string, handle_t> uniforms;

        /**
         * \brief Whether this program has been initially compiled and linked.
         */
        bool initialized;
        /**
         * \brief Whether this program must be rebuilt (due to the Shader
         *        list updating).
         */
        bool needs_rebuild;

        /**
         * \brief A handle to the linked program in video memory.
         */
        handle_t program_handle;
    };
}