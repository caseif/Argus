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

#include "argus/render/2d/camera_2d.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "internal/render/pimpl/common/scene.hpp"

#include <map>

namespace argus {
    // forward declarations
    struct pimpl_Scene;

    struct pimpl_Scene2D : public pimpl_Scene {
        RenderGroup2D *root_group_read;
        RenderGroup2D *root_group_write;

        std::map<Uuid, RenderGroup2D*> group_map;
        std::map<Uuid, RenderObject2D*> object_map;

        std::map<std::string, Camera2D> cameras;

        pimpl_Scene2D(const std::string &id, Scene2D &scene, const Transform2D &transform):
                pimpl_Scene(id, transform),
                root_group_read(new RenderGroup2D(scene, nullptr)),
                root_group_write(new RenderGroup2D(scene, nullptr)) {
        }

        pimpl_Scene2D(const pimpl_Scene2D&) = delete;

        pimpl_Scene2D(pimpl_Scene2D&&) = delete;
    };
}
