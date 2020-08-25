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
#include "internal/lowlevel/logging.hpp"

// module core
#include "internal/core/core_util.hpp"

// module render
#include "argus/render/render_group.hpp"
#include "argus/render/render_layer.hpp"
#include "argus/render/render_object.hpp"
#include "argus/render/render_prim.hpp"
#include "argus/render/renderer.hpp"
#include "argus/render/transform.hpp"
#include "internal/render/pimpl/render_group.hpp"
#include "internal/render/pimpl/render_layer.hpp"
#include "internal/render/pimpl/render_object.hpp"

#include <vector>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderLayer));

    RenderLayer::RenderLayer(const Renderer &parent, Transform &transform, const int index):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderLayer>(parent, transform, index)) {
        //TODO
    }

    RenderLayer::~RenderLayer(void) {
        g_pimpl_pool.free(pimpl);
    }
    
    RenderGroup &RenderLayer::create_child_group(Transform &transform) {
        auto group = new RenderGroup(*this, nullptr, transform);
        pimpl->child_groups.push_back(group);
        return *group;
    }

    RenderObject &RenderLayer::create_child_object(const Material &material, const std::vector<RenderPrim> &primitives,
            Transform &transform) {
        auto obj = new RenderObject(*this, nullptr, material, primitives, transform);
        pimpl->child_objects.push_back(obj);
        return *obj;
    }

    void RenderLayer::remove_child_group(RenderGroup &group) {
        if (&group.pimpl->parent_layer != this || group.pimpl->parent_group != nullptr) {
            throw std::invalid_argument("Supplied RenderGroup is not a direct child of the RenderLayer");
        }

        remove_from_vector(pimpl->child_groups, &group);
    }
    
    void RenderLayer::remove_child_object(RenderObject &object) {
        if (&object.pimpl->parent_layer != this || object.pimpl->parent_group != nullptr) {
            throw std::invalid_argument("Supplied RenderObject is not a direct child of the RenderLayer");
        }

        remove_from_vector(pimpl->child_objects, &object);
    }

    const Transform &RenderLayer::get_transform(void) {
        return pimpl->transform;
    }

    void RenderLayer::set_transform(Transform &transform) {
        pimpl->transform = transform;
    }

}
