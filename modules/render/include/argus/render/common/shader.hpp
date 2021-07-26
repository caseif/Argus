/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
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
    enum ShaderStage : uint32_t {
        VERTEX = 0x01,
        FRAGMENT = 0x02
    };

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
