/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/canvas.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"

#include "argus/game2d/world2d.hpp"
#include "internal/game2d/world2d.hpp"
#include "internal/game2d/world2d_layer.hpp"
#include "internal/game2d/pimpl/sprite.hpp"
#include "internal/game2d/pimpl/world2d.hpp"

#include <map>
#include <sstream>
#include <string>

#define FG_LAYER_ID "_foreground"
#define BG_LAYER_ID_PREFIX "_background_"

namespace argus {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_World2D));

    static std::map<std::string, World2D *> g_worlds;

    World2D &World2D::create(const std::string &id, Canvas &canvas, float scale_factor) {
        auto *world = new World2D(id, canvas, scale_factor);
        g_worlds.insert({id, world});
        return *world;
    }

    World2D &World2D::get(const std::string &id) {
        auto it = g_worlds.find(id);
        if (it == g_worlds.cend()) {
            throw std::invalid_argument("Unknown world ID");
        }

        return *it->second;
    }

    World2D::World2D(const std::string &id, Canvas &canvas, float scale_factor) :
            m_pimpl(&g_pimpl_pool.construct<pimpl_World2D>(id, canvas, scale_factor)) {
        m_pimpl->fg_layer = new World2DLayer(*this, FG_LAYER_ID, 1000, 1.0, std::nullopt, true);
    }

    World2D::~World2D(void) {
        g_pimpl_pool.destroy(m_pimpl);
    }

    const std::string &World2D::get_id(void) const {
        return m_pimpl->id;
    }

    float World2D::get_scale_factor(void) const {
        return m_pimpl->scale_factor;
    }

    const Transform2D &World2D::get_camera_transform(void) const {
        return m_pimpl->abstract_camera.peek();
    }

    void World2D::set_camera_transform(const Transform2D &transform) {
        m_pimpl->abstract_camera = transform;
    }

    float World2D::get_ambient_light_level(void) const {
        return m_pimpl->al_level.peek();
    }

    void World2D::set_ambient_light_level(float level) {
        m_pimpl->al_level = level;
    }

    Vector3f World2D::get_ambient_light_color(void) const {
        return m_pimpl->al_color.peek();
    }

    void World2D::set_ambient_light_color(const Vector3f &color) {
        m_pimpl->al_color = color;
    }

    World2DLayer &World2D::get_background_layer(uint32_t index) const {
        affirm_precond(index < m_pimpl->num_bg_layers, "Invalid background index requested");
        return *m_pimpl->bg_layers[index];
    }

    World2DLayer &World2D::add_background_layer(float parallax_coeff, std::optional<Vector2f> repeat_interval) {
        affirm_precond(m_pimpl->num_bg_layers < MAX_BACKGROUND_LAYERS, "Too many background layers added");

        auto bg_index = m_pimpl->num_bg_layers;

        std::stringstream layer_id;
        layer_id << BG_LAYER_ID_PREFIX;
        layer_id << bg_index;

        auto layer = new World2DLayer(*this, layer_id.str(), 100 + bg_index, parallax_coeff, repeat_interval, false);

        m_pimpl->bg_layers[bg_index] = layer;
        m_pimpl->num_bg_layers += 1;
        return *layer;
    }

    static void _render_world(World2D &world) {
        auto camera_transform = world.m_pimpl->abstract_camera.read();
        auto al_level = world.m_pimpl->al_level.read();
        auto al_color = world.m_pimpl->al_color.read();

        for (int64_t i = 0; i < world.m_pimpl->num_bg_layers; i++) {
            render_world_layer(*world.m_pimpl->bg_layers[i], camera_transform, al_level, al_color);
        }

        render_world_layer(*world.m_pimpl->fg_layer, camera_transform, al_level, al_color);
    }

    void render_worlds(TimeDelta delta) {
        UNUSED(delta);
        for (auto &[_, world] : g_worlds) {
            _render_world(*world);
        }
    }

    static World2DLayer &_get_foreground_layer(const World2D &world) {
        return *world.m_pimpl->fg_layer;
    }

    StaticObject2D &World2D::get_static_object(Handle handle) const {
        return _get_foreground_layer(*this).get_static_object(handle);
    }

    Handle World2D::create_static_object(const std::string &sprite, const Vector2f &size,
            uint32_t z_index, bool can_occlude_light, const Transform2D &transform) {
        return _get_foreground_layer(*this).create_static_object(sprite, size, z_index, can_occlude_light, transform);
    }

    void World2D::delete_static_object(Handle handle) {
        return _get_foreground_layer(*this).delete_static_object(handle);
    }

    Actor2D &World2D::get_actor(Handle handle) const {
        return _get_foreground_layer(*this).get_actor(handle);
    }

    Handle World2D::create_actor(const std::string &sprite, const Vector2f &size, uint32_t z_index,
            bool can_occlude_light, const Transform2D &transform) {
        return _get_foreground_layer(*this).create_actor(sprite, size, z_index, can_occlude_light, transform);
    }

    void World2D::delete_actor(Handle handle) {
        return _get_foreground_layer(*this).delete_actor(handle);
    }
}
