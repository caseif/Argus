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

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/render/defines.hpp"
#include "argus/render/common/canvas.hpp"
#include "argus/render/common/material.hpp"
#include "argus/render/common/texture_data.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"

#include "argus/game2d/animated_sprite.hpp"
#include "argus/game2d/sprite.hpp"
#include "argus/game2d/world2d.hpp"
#include "internal/game2d/world2d.hpp"
#include "internal/game2d/world2d_layer.hpp"
#include "internal/game2d/pimpl/animated_sprite.hpp"
#include "internal/game2d/pimpl/sprite.hpp"
#include "internal/game2d/pimpl/world2d.hpp"
#include "internal/game2d/defines.hpp"

#include <map>
#include <string>
#include <utility>

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_World2D));

    static std::map<std::string, World2D*> g_worlds;

    World2D &World2D::create(const std::string &id, Canvas &canvas, float scale_factor) {
        auto *world = new World2D(id, canvas, scale_factor);
        g_worlds.insert({ id, world });
        return *world;
    }

    World2D &World2D::get(const std::string &id) {
        auto it = g_worlds.find(id);
        if (it == g_worlds.cend()) {
            throw std::invalid_argument("Unknown world ID");
        }

        return *it->second;
    }

    World2D::World2D(const std::string &id, Canvas &canvas, float scale_factor):
            pimpl(&g_pimpl_pool.construct<pimpl_World2D>(id, canvas, scale_factor)) {
    }

    World2D::~World2D(void) {
        g_pimpl_pool.destroy(pimpl);
    }

    const std::string &World2D::get_id(void) const {
        return pimpl->id;
    }

    float World2D::get_scale_factor(void) const {
        return pimpl->scale_factor;
    }

    const Transform2D &World2D::get_camera_transform(void) const {
        return pimpl->abstract_camera.peek();
    }

    void World2D::set_camera_transform(const Transform2D &transform) {
        pimpl->abstract_camera = transform;
    }

    static void _render_world(World2D &world) {
        for (auto &layer : world.pimpl->layers) {
            render_world_layer(*layer);
        }
    }

    void render_worlds(TimeDelta delta) {
        UNUSED(delta);
        for (auto &kv : g_worlds) {
            auto &world = *kv.second;
            _render_world(world);
        }
    }

    World2DLayer &World2D::add_layer(const std::string &id) {
        auto *layer = new World2DLayer(*this, id, 1, std::nullopt);
        pimpl->layers.push_back(layer);
        return *layer;
    }

    const std::vector<World2DLayer*> &World2D::get_layers(void) const {
        return pimpl->layers;
    }
}
