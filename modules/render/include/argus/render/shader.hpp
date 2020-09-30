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
     * \brief Represents a shader for use with a RenderGroup or RenderLayer.
     *
     * Because of limitations in the low-level graphics API, Argus requires that
     * each shader specify an entry point other than main(). When shaders are
     * built, a main() function is generated containing calls to each shader's
     * respective entry point.
     */
    class Shader {
        public:
            pimpl_Shader *const pimpl;

            /**
             * \brief Constructs a new Shader with the given parameters.
             *
             * \param type The type of the Shader as a magic value.
             * \param src The source data of the Shader.
             * \param src_len The length of the Shader's source data.
             * \param entry_point The entry point of the Shader.
             * \param order The order value of the Shader.
             * \param uniform_ids A list of uniforms defined by this shader.
             *
             * \remark Shaders with a lower order value are executed before ones
                       with higher order values within their respective stage.
             */
            Shader(const unsigned int type, const char *const src, const size_t src_len,
                    const std::string &entry_point, const int order,
                    const std::initializer_list<std::string> &uniform_ids);

            /**
             * \brief Creates a new vertex shader with the given parameters.
             *
             * \param src The source data of the Shader.
             * \param src_len The length of the Shader's source data.
             * \param entry_point The entry point of the Shader.
             * \param order The order value of the Shader.
             * \param uniform_ids A list of uniforms defined by this shader.
             *
             * \remark Shaders with a lower order value are executed before ones
                       with higher order values within their respective stage.
             *
             * \return The constructed vertex Shader.
             */
            static Shader create_vertex_shader(const char *const src, const size_t src_len,
                    const std::string &entry_point, const int order,
                    const std::initializer_list<std::string> &uniform_ids);

            /**
             * \brief Creates a new fragment shader with the given parameters.
             *
             * \param src The source data of the Shader.
             * \param src_len The length of the Shader's source data.
             * \param entry_point The entry point of the Shader.
             * \param order The order value of the Shader.
             * \param uniform_ids A list of uniforms defined by this shader.
             *
             * \remark Shaders with a lower order value are executed before ones
                       with higher order values within their respective stage.
             *
             * \return The constructed fragment Shader.
             */
            static Shader create_fragment_shader(const char *const src, const size_t src_len,
                    const std::string &entry_point, const int order,
                    const std::initializer_list<std::string> &uniform_ids);
    };
}
