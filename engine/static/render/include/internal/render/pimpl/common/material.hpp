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

// module render
#include "argus/render/common/material.hpp"

#include <atomic>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Shader;
    class TextureData;

    struct pimpl_Material {
        const std::string texture;
        const std::vector<std::string> shaders;
        const VertexAttributes attributes;

        pimpl_Material(const std::string &texture, const std::vector<std::string> &shaders,
            const VertexAttributes attributes):
            texture(texture),
            shaders(shaders),
            attributes(attributes) {
        }

        pimpl_Material(const pimpl_Material&) = default;

        pimpl_Material(pimpl_Material&&) = delete;
    };
}
