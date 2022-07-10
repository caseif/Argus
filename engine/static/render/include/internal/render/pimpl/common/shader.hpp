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

#pragma once

#include "argus/render/common/shader.hpp"

#include <initializer_list>
#include <string>
#include <vector>

#include <cstdint>

namespace argus {
    struct pimpl_Shader {
        /**
         * \brief The unique identifier of the shader.
         */
        const std::string uid;
        /**
         * \brief The type of shader stored by this object as a magic ID.
         */
        const std::string type;
        /**
         * \brief The stage this shader is to be run at.
         */
        const ShaderStage stage;
        /**
         * \brief The source code for this shader.
         */
        const std::vector<uint8_t> src;
        /**
         * \brief The reflection information for this shader.
         */
        const ShaderReflectionInfo reflection;

        pimpl_Shader(const std::string &uid, const std::string &type, ShaderStage stage,
            const std::vector<uint8_t> &src, const ShaderReflectionInfo &reflection):
                uid(uid),
                type(type),
                stage(stage),
                src(src),
                reflection(reflection) {
        }

        pimpl_Shader(const pimpl_Shader&) = default;

        pimpl_Shader(pimpl_Shader&&) = delete;
    };
}
