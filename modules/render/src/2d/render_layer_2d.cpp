/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include <stdexcept>

// module lowlevel
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/common/render_layer_type.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_layer_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "internal/render/pimpl/common/render_layer.hpp"
#include "internal/render/pimpl/2d/render_layer_2d.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderLayer2D));

    RenderLayer2D::RenderLayer2D(const Renderer &parent, const Transform2D &transform, const int index):
        RenderLayer(RenderLayerType::Render2D),
        pimpl(&g_pimpl_pool.construct<pimpl_RenderLayer2D>(parent, *this, transform, index)) {
    }

    RenderLayer2D::RenderLayer2D(const RenderLayer2D &rhs) noexcept:
        RenderLayer(RenderLayerType::Render2D),
        pimpl(&g_pimpl_pool.construct<pimpl_RenderLayer2D>(*rhs.pimpl)) {
    }

    RenderLayer2D::RenderLayer2D(RenderLayer2D &&rhs) noexcept:
        RenderLayer(RenderLayerType::Render2D),
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    RenderLayer2D::~RenderLayer2D(void) {
        if (pimpl != nullptr) {
            pimpl->root_group.~RenderGroup2D();

            g_pimpl_pool.free(pimpl);
        }
    }

    pimpl_RenderLayer *RenderLayer2D::get_pimpl(void) const {
        return dynamic_cast<pimpl_RenderLayer*>(pimpl);
    }

    RenderGroup2D &RenderLayer2D::create_child_group(const Transform2D &transform) {
        return pimpl->root_group.create_child_group(transform);
    }

    RenderObject2D &RenderLayer2D::create_child_object(const Material &material,
            const std::vector<RenderPrim2D> &primitives, const Transform2D &transform) {
        return pimpl->root_group.create_child_object(material, primitives, transform);
    }

    void RenderLayer2D::remove_child_group(RenderGroup2D &group) {
        if (group.get_parent_group() != &pimpl->root_group) {
            throw std::invalid_argument("Supplied RenderGroup2D is not a direct child of the RenderLayer2D");
        }

        pimpl->root_group.remove_child_group(group);
    }

    void RenderLayer2D::remove_child_object(RenderObject2D &object) {
        if (&object.pimpl->parent_group != &pimpl->root_group) {
            throw std::invalid_argument("Supplied RenderObject2D is not a direct child of the RenderLayer2D");
        }

        pimpl->root_group.remove_child_object(object);
    }
}
