/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/material.hpp"

#include <atomic>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Shader;
    class TextureData;

    struct pimpl_Material {
        const std::string id;
        const TextureData &texture;
        const std::vector<const Shader*> shaders;
        const VertexAttributes attributes;

        pimpl_Material(const std::string id, const TextureData &texture, const std::vector<const Shader*> &shaders,
            const VertexAttributes attributes):
            id(id),
            texture(texture),
            shaders(shaders),
            attributes(attributes) {
        }

        pimpl_Material(const pimpl_Material&) = default;

        pimpl_Material(pimpl_Material&&) = delete;
    };
}
