/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "argus/render/material.hpp"

#include <atomic>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class ShaderProgram;

    struct pimpl_Material {
        const TextureData &texture;

        const std::vector<Shader*> shaders;
        ShaderProgram *program;

        const std::vector<VertexAttribDef> vertex_attribs;

        pimpl_Material(const TextureData &texture, const std::vector<Shader*> &shaders,
            const std::vector<VertexAttribDef> vertex_attribs):
            texture(texture),
            shaders(shaders),
            vertex_attribs(vertex_attribs) {
        }
    };

}
