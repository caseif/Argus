/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
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

    /**
     * Represents attributes to be enabled 
     */
    enum VertexAttributes : uint16_t {
        NONE = 0x0,
        POSITION = 0x1,
        NORMAL = 0x2,
        COLOR = 0x4,
        TEXCOORD = 0x8
    };

    constexpr inline VertexAttributes operator|(const VertexAttributes lhs, const VertexAttributes rhs) {
        return static_cast<VertexAttributes>(static_cast<std::underlying_type<VertexAttributes>::type>(lhs) |
                                             static_cast<std::underlying_type<VertexAttributes>::type>(rhs));
    }

    inline VertexAttributes operator|=(VertexAttributes &lhs, const VertexAttributes rhs) {
        return lhs = static_cast<VertexAttributes>(static_cast<std::underlying_type<VertexAttributes>::type>(lhs) |
                                             static_cast<std::underlying_type<VertexAttributes>::type>(rhs));
    }

    class Material {
        public:
            pimpl_Material *pimpl;

            /**
             * \brief Constructs a new Material.
             *
             * \param texture The texture used by the Material.
             * \param shaders The shaders used by the Material. Only one Shader
             *        may be specified per \link ShaderStage shader stage \endlink.
             * \param attributes The vertex attributes used by this material.
             */
            Material(const TextureData &texture, const std::vector<const Shader*> &shaders,
                    const VertexAttributes &attributes);

            Material(const Material&) noexcept;

            Material(Material&&) noexcept;

            ~Material(void);
    };

}
