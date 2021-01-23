/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
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
#include "argus/render/renderer.hpp"
#include "argus/render/transform.hpp"
#include "internal/render/pimpl/render_layer.hpp"
#include "internal/render/pimpl/render_object.hpp"

#include <stdexcept>
#include <vector>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderLayer));

    RenderLayer::RenderLayer(const Renderer &parent, Transform &transform, const int index):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderLayer>(parent, *this, transform, index)) {
    }

    RenderLayer::RenderLayer(const RenderLayer &rhs) noexcept:
        pimpl(&g_pimpl_pool.construct<pimpl_RenderLayer>(*rhs.pimpl)) {
    }

    RenderLayer::RenderLayer(RenderLayer &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    RenderLayer::~RenderLayer(void) {
        if (pimpl != nullptr) {
            g_pimpl_pool.free(pimpl);
        }
    }

    const Renderer &RenderLayer::get_parent_renderer(void) const {
        return pimpl->parent_renderer;
    }
    
    RenderGroup &RenderLayer::create_child_group(Transform &transform) {
        return pimpl->root_group.create_child_group(transform);
    }

    RenderObject &RenderLayer::create_child_object(const Material &material, const std::vector<RenderPrim> &primitives,
            Transform &transform) {
        return pimpl->root_group.create_child_object(material, primitives, transform);
    }

    void RenderLayer::remove_child_group(RenderGroup &group) {
        if (group.get_parent_group() != &pimpl->root_group) {
            throw std::invalid_argument("Supplied RenderGroup is not a direct child of the RenderLayer");
        }

        pimpl->root_group.remove_child_group(group);
    }
    
    void RenderLayer::remove_child_object(RenderObject &object) {
        if (&object.pimpl->parent_group != &pimpl->root_group) {
            throw std::invalid_argument("Supplied RenderObject is not a direct child of the RenderLayer");
        }

        pimpl->root_group.remove_child_object(object);
    }

    Transform &RenderLayer::get_transform(void) const {
        return pimpl->transform;
    }

    void RenderLayer::set_transform(Transform &transform) {
        pimpl->transform = transform;
    }

}
