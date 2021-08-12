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
#include "argus/render/common/material.hpp"
#include "argus/render/common/transform.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_layer_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"

#include <atomic>
#include <vector>

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderObject2D));

    RenderObject2D::RenderObject2D(const RenderGroup2D &parent_group, const std::string &material,
            const std::vector<RenderPrim2D> &primitives, const Transform2D &transform):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderObject2D>(parent_group, material, primitives, transform)) {
    }

    RenderObject2D::RenderObject2D(const RenderObject2D &rhs) noexcept:
        pimpl(&g_pimpl_pool.construct<pimpl_RenderObject2D>(*rhs.pimpl)) {
    }

    RenderObject2D::RenderObject2D(RenderObject2D &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    RenderObject2D::~RenderObject2D() {
        if (pimpl != nullptr) {
            g_pimpl_pool.destroy(pimpl);
        }
    }

    const RenderLayer2D &RenderObject2D::get_parent_layer(void) const {
        return pimpl->parent_group.get_parent_layer();
    }

    std::string RenderObject2D::get_material(void) const {
        return pimpl->material;
    }

    const std::vector<RenderPrim2D> &RenderObject2D::get_primitives(void) const {
        return pimpl->primitives;
    }

    Transform2D &RenderObject2D::get_transform(void) const {
        return pimpl->transform;
    }

    void RenderObject2D::set_transform(Transform2D &transform) const {
        pimpl->transform = transform;
        pimpl->transform.pimpl->dirty = true;
    }
}
