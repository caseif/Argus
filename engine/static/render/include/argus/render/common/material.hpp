/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

    class Material {
        friend class MaterialLoader;

        public:
            pimpl_Material *pimpl;

            /**
             * \brief Constructs a new Material.
             *
             * \param texture The UID of the texture used by the Material.
             * \param shaders The UIDs of the shaders used by the Material. Only
             *        one Shader may be specified per \link ShaderStage shader
             *        stage \endlink.
             */
            Material(const std::string &texture, const std::vector<std::string> &shaders);

            Material(const Material&) noexcept;

            Material(Material&&) noexcept;

            ~Material(void);

            const std::string &get_texture_uid(void) const;

            const std::vector<std::string> &get_shader_uids(void) const;
    };

}
