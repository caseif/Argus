/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "argus/lowlevel/dirtiable.hpp"

#include "argus/render/2d/render_group_2d.hpp"

#include <map>
#include <vector>

namespace argus {
    struct pimpl_RenderGroup2D {
        std::string id;
        Scene2D &scene;
        RenderGroup2D *parent_group;
        Transform2D transform;
        std::vector<RenderGroup2D*> child_groups;
        std::vector<RenderObject2D*> child_objects;

        pimpl_RenderGroup2D(const std::string &id, Scene2D &scene, RenderGroup2D *const parent_group,
                const Transform2D &transform):
            id(id),
            scene(scene),
            parent_group(parent_group),
            transform(transform) {
        }

        pimpl_RenderGroup2D(const std::string &id, Scene2D &scene, RenderGroup2D *const parent_group,
                Transform2D &&transform):
            id(id),
            scene(scene),
            parent_group(parent_group),
            transform(transform) {
        }

        pimpl_RenderGroup2D(const std::string &id, Scene2D &scene, RenderGroup2D *const parent_group):
            id(id),
            scene(scene),
            parent_group(parent_group) {
        }

        pimpl_RenderGroup2D(pimpl_RenderGroup2D&) = default;

        pimpl_RenderGroup2D(pimpl_RenderGroup2D&&) = delete;
    };
}
