/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
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

    constexpr inline ShaderStage operator|=(const ShaderStage lhs, const ShaderStage rhs) {
        return static_cast<ShaderStage>(static_cast<std::underlying_type<ShaderStage>::type>(lhs) |
                                        static_cast<std::underlying_type<ShaderStage>::type>(rhs));
    }

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
             * \param src_len The length of the Shader's source data.
             */
            Shader(const ShaderStage stage, const char *const src, const size_t src_len);

            Shader(const Shader&) noexcept;

            Shader(Shader&&) noexcept;

            ~Shader(void);

            /**
             * \brief Creates a new Shader with the given parameters.
             *
             * \param stage The stage of the graphics pipeline this shader is to
*                     be run at.
             * \param src The source data of the Shader.
             * \param src_len The length of the Shader's source data.
             *
             * \return The constructed Shader.
             */
            static Shader create_shader(const ShaderStage stage, const char *const src, const size_t src_len);
    };
}
