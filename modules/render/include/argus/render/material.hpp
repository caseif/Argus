/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <string>
#include <typeinfo>
#include <vector>

#include <cstddef>

namespace argus {
    // forward declarations
    class Renderer;
    class Shader;
    class TextureData;

    struct pimpl_Material;

    struct VertexAttribDef {
        std::string id;
        char type;
        size_t count;
    };

    class Material {
        public:
            pimpl_Material *const pimpl;

            /**
             * \brief Constructs a new Material.
             *
             * \param id The unique ID of the material.
             */
            Material(const TextureData &texture, const std::vector<Shader*> &shaders,
                const std::vector<VertexAttribDef> &vertex_attribs);

            ~Material(void);

            void initialize(void);
    };

}
