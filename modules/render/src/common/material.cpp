/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/common/material.hpp"
#include "argus/render/common/shader.hpp"
#include "argus/render/common/texture_data.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/renderer_impl.hpp"
#include "internal/render/pimpl/common/material.hpp"
#include "internal/render/pimpl/common/shader.hpp"

#include <string>
#include <vector>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_Material));

    Material::Material(const TextureData &texture, const std::vector<const Shader*> &shaders,
            const VertexAttributes &attribs):
        pimpl(&g_pimpl_pool.construct<pimpl_Material>(texture, shaders, attribs)) {
        ShaderStage seen = static_cast<ShaderStage>(0);
        for (const Shader *shader : shaders) {
            if (seen & shader->pimpl->stage) {
                _ARGUS_FATAL("Multiple shaders supplied for single stage\n");
            }
            //TODO: need a custom |= operator for ShaderStage
            seen = (ShaderStage) (seen | shader->pimpl->stage);
        }
    }

    Material::Material(const Material &rhs) noexcept:
        pimpl(&g_pimpl_pool.construct<pimpl_Material>(*rhs.pimpl)) {
    }

    Material::Material(Material &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Material::~Material(void) {
        if (pimpl != nullptr) {
            get_renderer_impl().deinit_material(*this);
            g_pimpl_pool.free(pimpl);
        }
    }
}
