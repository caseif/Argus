/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/material.hpp"
#include "argus/render/render_group.hpp"
#include "argus/render/render_layer.hpp"
#include "argus/render/render_object.hpp"
#include "argus/render/render_prim.hpp"
#include "argus/render/transform.hpp"
#include "internal/render/pimpl/render_object.hpp"
#include "internal/render/pimpl/transform.hpp"

#include <atomic>
#include <vector>

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderObject));

    RenderObject::RenderObject(const RenderGroup &parent_group, const Material &material,
            const std::vector<RenderPrim> &primitives, Transform &transform):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderObject>(parent_group, material, primitives, transform)) {
    }

    RenderObject::RenderObject(const RenderObject &rhs) noexcept:
        pimpl(&g_pimpl_pool.construct<pimpl_RenderObject>(*rhs.pimpl)) {
    }

    RenderObject::RenderObject(RenderObject &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    RenderObject::~RenderObject() {
        if (pimpl != nullptr) {
            g_pimpl_pool.free(pimpl);
        }
    }

    const RenderLayer &RenderObject::get_parent_layer(void) const {
        return pimpl->parent_group.get_parent_layer();
    }

    const Material &RenderObject::get_material(void) const {
        return pimpl->material;
    }

    const std::vector<RenderPrim> &RenderObject::get_primitives(void) const {
        return pimpl->primitives;
    }

    Transform &RenderObject::get_transform(void) const {
        return pimpl->transform;
    }

    void RenderObject::set_transform(Transform &transform) const {
        pimpl->transform = transform;
        pimpl->transform.pimpl->dirty = true;
    }
}
