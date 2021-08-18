/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/common/scene.hpp"
#include "argus/render/common/transform.hpp"

#include <vector>

namespace argus {
    struct pimpl_Scene {
        const Renderer &parent_renderer;
        Transform2D transform;
        const int index;
        
        pimpl_Scene(const Renderer &renderer, const Transform2D &transform, const int index):
                parent_renderer(renderer),
                transform(transform),
                index(index) {
        }

        pimpl_Scene(const pimpl_Scene&) = default;

        pimpl_Scene(pimpl_Scene&&) = delete;
    };
}
