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
#include "argus/render/2d/render_group_2d.hpp"

#include <map>
#include <vector>

namespace argus {
    struct pimpl_RenderGroup2D {
        Scene2D &scene;
        RenderGroup2D *const parent_group;
        Transform2D transform;
        std::vector<RenderGroup2D*> child_groups;
        std::vector<RenderObject2D*> child_objects;

        pimpl_RenderGroup2D(Scene2D &scene, RenderGroup2D *const parent_group, const Transform2D &transform):
                scene(scene),
                parent_group(parent_group),
                transform(transform) {
        }

        pimpl_RenderGroup2D(Scene2D &scene, RenderGroup2D *const parent_group, Transform2D &&transform):
                scene(scene),
                parent_group(parent_group),
                transform(transform) {
        }

        pimpl_RenderGroup2D(Scene2D &scene, RenderGroup2D *const parent_group):
                scene(scene),
                parent_group(parent_group) {
        }

        pimpl_RenderGroup2D(pimpl_RenderGroup2D&) = default;

        pimpl_RenderGroup2D(pimpl_RenderGroup2D&&) = delete;
    };
}
