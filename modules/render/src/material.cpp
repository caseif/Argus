/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module resman
#include "argus/resman.hpp"

// module render
#include "argus/render/material.hpp"
#include "argus/render/shader.hpp"
#include "argus/render/texture_data.hpp"
#include "internal/render/pimpl/material.hpp"

#include <vector>

namespace argus {

    Material::Material(const TextureData &texture, const std::vector<Shader*> &shaders,
            const std::vector<VertexAttribDef> &vertex_attribs):
        pimpl(new pimpl_Material(texture, shaders, vertex_attribs)) {
        //TODO
    }

    void Material::initialize(void) {
        //TODO
    }

}
