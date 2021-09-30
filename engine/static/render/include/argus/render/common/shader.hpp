/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

#include <initializer_list>
#include <string>

namespace argus {
    // forward declarations
    struct pimpl_Shader;
    
    /**
     * \brief Represents a stage corresponding to a step in the render pipeline.
     */
    enum class ShaderStage : uint32_t {
        Vertex = 0x01,
        Fragment = 0x02
    };

    ShaderStage operator |(ShaderStage lhs, ShaderStage rhs);

    ShaderStage &operator |=(ShaderStage &lhs, ShaderStage rhs);

    ShaderStage operator &(ShaderStage lhs, ShaderStage rhs);

    /**
     * \brief Represents a shader for use with a RenderObject.
     *
     * Because of limitations in the low-level graphics API, Argus requires that
     * each shader specify an entry point other than main(). When shaders are
     * built, a main() function is generated containing calls to each shader's
     * respective entry point.
     */
    class Shader {
        public:
            pimpl_Shader *pimpl;

            /**
             * \brief Constructs a new Shader with the given parameters.
             *
             * \param stage The stage of the graphics pipeline this shader is to
*                     be run at.
             * \param src The source data of the Shader.
             */
            Shader(ShaderStage stage, const std::string &src);

            Shader(const Shader&) noexcept;

            Shader(Shader&&) noexcept;

            ~Shader(void);

            ShaderStage get_stage(void) const;
    };
}