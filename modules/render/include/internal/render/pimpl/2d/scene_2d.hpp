/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "internal/render/pimpl/common/scene.hpp"

namespace argus {
    // forward declarations
    struct pimpl_Scene;

    struct pimpl_Scene2D : public pimpl_Scene {
        RenderGroup2D root_group;
        
        pimpl_Scene2D(const Renderer &renderer, Scene2D &scene, const Transform2D &transform,
                const int index):
                pimpl_Scene(renderer, transform, index),
                root_group(scene, nullptr) {
        }

        pimpl_Scene2D(const pimpl_Scene2D&) = default;

        pimpl_Scene2D(pimpl_Scene2D&&) = delete;
    };
}
