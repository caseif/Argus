/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/memory.hpp"

// module render
#include "argus/render/material.hpp"
#include "argus/render/render_group.hpp"
#include "argus/render/render_layer.hpp"
#include "argus/render/render_object.hpp"
#include "argus/render/render_prim.hpp"
#include "argus/render/transform.hpp"
#include "internal/render/pimpl/render_object.hpp"

#include <vector>

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderObject));

    RenderObject::RenderObject(RenderLayer &parent_layer, RenderGroup *const parent_group,
            const Material &material, const std::vector<RenderPrim> &primitives,
            Transform &transform):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderObject>(parent_layer, parent_group, material, primitives, transform)) {
    }

    RenderObject::~RenderObject() {
        //TODO
    }

    const Material &RenderObject::get_material(void) const {
        return pimpl->material;
    }

    const std::vector<RenderPrim> &RenderObject::get_primitives(void) const {
        return pimpl->primitives;
    }

    const Transform &RenderObject::get_transform(void) const {
        return pimpl->transform;
    }

    void RenderObject::set_transform(Transform &transform) const {
        pimpl->transform = transform;
    }
}