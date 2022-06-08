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

#include <string>
#include <typeinfo>
#include <vector>

#include <cstddef>

namespace argus {
    // forward declarations
    class Canvas;
    class Shader;
    class TextureData;

    struct pimpl_Material;

    /**
     * Represents attributes to be enabled 
     */
    enum VertexAttributes : uint64_t {
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
        friend class MaterialLoader;

        public:
            pimpl_Material *pimpl;

        private:
            /**
             * \brief Constructs a new Material.
             *
             * \param texture The UID of the texture used by the Material.
             * \param shaders The UIDs of the shaders used by the Material. Only
             *        one Shader may be specified per \link ShaderStage shader
             *        stage \endlink.
             * \param attributes The vertex attributes used by this material.
             */
            Material(const std::string &texture, const std::vector<std::string> &shaders,
                    const VertexAttributes &attributes);

            Material(const Material&) noexcept;

            Material(Material&&) noexcept;

            ~Material(void);

        public:
            const std::string &get_texture_uid(void) const;

            const std::vector<std::string> &get_shader_uids(void) const;
        
            VertexAttributes get_vertex_attributes(void) const;
    };

}
