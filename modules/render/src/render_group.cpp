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

// module core
#include "internal/core/core_util.hpp"

// module resman
#include "argus/resman.hpp"

// module render
#include "argus/render/material.hpp"
#include "argus/render/render_group.hpp"
#include "argus/render/render_layer.hpp"
#include "argus/render/render_object.hpp"
#include "argus/render/transform.hpp"
#include "internal/render/pimpl/render_group.hpp"
#include "internal/render/pimpl/render_object.hpp"

#include <stdexcept>
#include <vector>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderGroup));

    RenderGroup::RenderGroup(const RenderLayer &parent_layer, RenderGroup *const parent_group, Transform &transform):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup>(parent_layer, parent_group, transform)) {
        //TODO
    }

    RenderGroup::~RenderGroup(void) {
        g_pimpl_pool.free(pimpl);
    }

    const RenderLayer &RenderGroup::get_parent_layer(void) const {
        return pimpl->parent_layer;
    }

    RenderGroup *const RenderGroup::get_parent_group(void) const {
        return pimpl->parent_group;
    }

    RenderGroup &RenderGroup::create_child_group(Transform &transform) {
        auto group = new RenderGroup(pimpl->parent_layer, this, transform);
        pimpl->child_groups.push_back(group);
        return *group;
    }

    RenderObject &RenderGroup::create_child_object(const Material &material, const std::vector<RenderPrim> &primitives,
            Transform &transform) {
        auto obj = new RenderObject(*this, material, primitives, transform);
        pimpl->child_objects.push_back(obj);
        return *obj;
    }

    void RenderGroup::remove_child_group(RenderGroup &group) {
        if (group.pimpl->parent_group != this) {
            throw std::invalid_argument("Supplied RenderGroup is not a child of RenderGroup");
        }

        remove_from_vector(pimpl->child_groups, &group);
    }

    void RenderGroup::remove_child_object(RenderObject &object) {
        if (&object.pimpl->parent_group != this) {
            throw std::invalid_argument("Supplied RenderObject is not a child of RenderGroup");
        }

        remove_from_vector(pimpl->child_objects, &object);
    }

    Transform &RenderGroup::get_transform(void) const {
        return pimpl->transform;
    }

    void RenderGroup::set_transform(Transform &transform) {
        pimpl->transform = transform;
    }
}
