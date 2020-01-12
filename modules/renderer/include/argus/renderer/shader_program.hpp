/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module renderer
#include "argus/renderer/render_group.hpp"
#include "argus/renderer/render_layer.hpp"
#include "argus/renderer/shader.hpp"
#include "argus/renderer/util/types.hpp"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace argus {
    // forward declarations
    class RenderGroup;
    class RenderLayer;
    class Shader;

    /**
     * \brief Represents a linked shader program for use with a RenderGroup.
     */
    class ShaderProgram {
        friend class RenderLayer;
        friend class RenderGroup;

        private:
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

            /**
             * \brief Constructs a new ShaderProgram encompassing the given
             *        Shaders.
             *
             * \param shaders The \link Shader Shaders \endlink to construct the
             *        new program with.
             */
            ShaderProgram(const std::vector<const Shader*> &shaders);

            /**
             * \brief Compiles and links this program so it may be used in
             *        rendering.
             */
            void link(void);

            /**
             * \brief Updates the list of Shaders encompassed by this program.
             *
             * \param shaders The new list of Shaders for this program.
             */
            void update_shaders(const std::vector<const Shader*> &shaders);

            /**
             * \brief Updates this program's implicit projection matrix uniform
             *        to match the given dimensions.
             *
             * \param viewport_width The new width of the viewport.
             * \param viewport_height The new height of the viewport.
             */
            void update_projection_matrix(const unsigned int viewport_width, const unsigned int viewport_height);

        public:
            /**
             * \brief Gets a handle to a given uniform defined by this program.
             *
             * \param uniform_id The ID string of the uniform to look up.
             *
             * \return The location of the uniform.
             *
             * \warning Invoking this method with a non-present uniform ID will
             *          trigger a fatal engine error.
             *
             * \deprecated This will be removed after functions are added to
             *             abstract the setting of uniforms.
             */
            handle_t get_uniform_location(const std::string &uniform_id) const;

            //TODO: uniform set abstraction
    };
}
