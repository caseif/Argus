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
#include "argus/render/renderer.hpp"
#include "argus/render/transform.hpp"
#include "internal/render/pimpl/render_layer.hpp"

#include <vector>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderLayer));

    RenderLayer::RenderLayer(const Renderer &parent, Transform &transform, const int index):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderLayer>(parent, transform, index)) {
        //TODO
    }
    
    RenderGroup &RenderLayer::create_child_group(const Transform &transform) {
        //TODO
    }

    RenderObject &RenderLayer::create_child_object(const Material &material,
            const std::vector<RenderPrim> &primitives, const Transform &transform) {
        //TODO
    }

    void RenderLayer::remove_child_group(RenderGroup &group) {
        //TODO
    }
    
    void RenderLayer::remove_child_object(RenderObject &object) {
        //TODO
    }

    Transform &RenderLayer::get_transform(void) {
        //TODO
    }

    void RenderLayer::set_transform(Transform &transform) {
        //TODO
    }

}
