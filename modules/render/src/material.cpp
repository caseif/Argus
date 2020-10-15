/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module resman
#include "argus/resman.hpp"

// module render
#include "argus/render/material.hpp"
#include "argus/render/shader.hpp"
#include "argus/render/texture_data.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/renderer_impl.hpp"
#include "internal/render/pimpl/material.hpp"
#include "internal/render/pimpl/shader.hpp"

#include <vector>

namespace argus {

    Material::Material(const std::string id, const TextureData &texture, const std::vector<const Shader*> &shaders,
            const VertexAttributes &attribs):
        pimpl(new pimpl_Material(id, texture, shaders, attribs)) {
        ShaderStage seen = static_cast<ShaderStage>(0);
        for (const Shader *shader : shaders) {
            if (seen & shader->pimpl->stage) {
                _ARGUS_FATAL("Multiple shaders supplied for single stage\n");
            }
            seen |= shader->pimpl->stage;
        }
    }

    Material::~Material(void) {
        get_renderer_impl().deinit_material(*this);
    }

    const std::string Material::get_id(void) const {
        return pimpl->id;
    }

}
