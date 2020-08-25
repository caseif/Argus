/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/math.hpp"
#include "argus/memory.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "internal/core/core_util.hpp"

// module resman
#include "argus/resource_manager.hpp"

// module render
#include "argus/render/render_group.hpp"
#include "argus/render/render_layer.hpp"
#include "argus/render/render_object.hpp"
#include "argus/render/transform.hpp"
#include "internal/render/pimpl/render_group.hpp"


namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderGroup));

    RenderGroup::RenderGroup(RenderLayer &parent_layer, RenderGroup *const parent_group,
                    Transform &transform):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup>(parent_layer, parent_group, transform)) {
        //TODO
    }

    RenderGroup *const RenderGroup::get_parent_group(void) const {
        //TODO
    }

    RenderGroup &RenderGroup::create_child_group(Transform &transform) {
        //TODO
    }

    RenderObject &RenderGroup::create_child_object(const Material &material, const std::vector<RenderPrim> &primitives,
            Transform &transform) {
        //TODO
    }

    void RenderGroup::remove_child_group(RenderGroup &group) {
        //TODO
    }

    void RenderGroup::remove_child_object(RenderObject &object) {
        //TODO
    }

    const Transform &RenderGroup::get_transform(void) const {
        //TODO
    }

    void RenderGroup::set_transform(Transform &transform) {
        //TODO
    }
}
