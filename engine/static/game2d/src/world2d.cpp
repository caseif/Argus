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

#include "argus/render/common/canvas.hpp"
#include "argus/render/common/material.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"

#include "argus/game2d/animated_sprite.hpp"
#include "argus/game2d/sprite.hpp"
#include "argus/game2d/world2d.hpp"
#include "internal/game2d/world2d.hpp"
#include "internal/game2d/pimpl/world2d.hpp"
#include "argus/render/defines.hpp"
#include "internal/game2d/defines.hpp"

#include <map>
#include <string>
#include <utility>

#define WORLD_PREFIX "_world_"
#define SPRITE_PREFIX "_sprite_"

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_World2D));

    static std::map<std::string, World2D*> g_worlds;

    World2D &World2D::create_world_2d(const std::string &id, Canvas &canvas) {
        auto *world = new World2D(id, canvas);
        g_worlds.insert({ id, world });
        return *world;
    }

    World2D::World2D(const std::string &id, Canvas &canvas):
            pimpl(&g_pimpl_pool.construct<pimpl_World2D>(id, canvas)) {
        pimpl->scene = &Scene2D::create(WORLD_PREFIX + id);
        pimpl->camera = &pimpl->scene->create_camera(WORLD_PREFIX + id);
        canvas.attach_default_viewport_2d(WORLD_PREFIX + id, *pimpl->camera);
    }

    World2D::~World2D(void) {
        g_pimpl_pool.destroy(pimpl);
    }

    Camera2D &World2D::get_camera(void) const {
        return *pimpl->camera;
    }

    std::optional<std::reference_wrapper<Sprite>> World2D::get_sprite(const std::string &id) const {
        auto it = pimpl->sprites.find(id);
        return it != pimpl->sprites.end() ? std::make_optional(std::reference_wrapper(*it->second)) : std::nullopt;
    }

    Sprite &World2D::add_sprite(const std::string &id, const Vector2f &base_size, const std::string &texture_uid,
                       const std::pair<Vector2f, Vector2f> tex_coords) {
        if (pimpl->sprites.find(id) != pimpl->sprites.cend()) {
            throw std::invalid_argument("Sprite with ID already exists in world");
        }

        auto *sprite = new Sprite(id, base_size, texture_uid, tex_coords);
        pimpl->sprites.insert({ id, sprite });

        return *sprite;
    }

    void World2D::remove_sprite(const std::string &id) {
        pimpl->sprites.erase(id);
    }

    std::optional<std::reference_wrapper<AnimatedSprite>> World2D::get_animated_sprite(const std::string &id) const {
        auto it = pimpl->anim_sprites.find(id);
        return it != pimpl->anim_sprites.cend()
                ? std::make_optional(std::reference_wrapper(*it->second))
                : std::nullopt;
    }

    AnimatedSprite &World2D::add_animated_sprite(const std::string &id, const std::string &sprite_uid) {
        if (pimpl->anim_sprites.find(id) != pimpl->anim_sprites.cend()) {
            throw std::invalid_argument("Animated sprite with given ID already exists in the world");
        }

        auto &def_res = ResourceManager::instance().get_resource(sprite_uid);

        auto *sprite = new AnimatedSprite(def_res);

        pimpl->anim_sprites.insert({ id, sprite });

        return *sprite;
    }

    void World2D::remove_animated_sprite(const std::string &id) {
        if (pimpl->anim_sprites.find(id) == pimpl->anim_sprites.cend()) {
            Logger::default_logger().warn("Client attempted to remove non-existent sprite ID %s", id.c_str());
            return;
        }

        pimpl->anim_sprites.erase(id);
    }

    static void _render_sprite(World2D &world, Sprite &sprite) {
        UNUSED(sprite);
        auto obj_opt = world.pimpl->scene->get_object(SPRITE_PREFIX + sprite.get_id());
        if (!obj_opt.has_value()) {
            std::vector<RenderPrim2D> prims;

            Vertex2D v1, v2, v3, v4;

            v1.position = { 0, 0 };
            v2.position = { 0, sprite.get_base_size().y };
            v3.position = { sprite.get_base_size().x, sprite.get_base_size().y };
            v4.position = { sprite.get_base_size().x, 0 };

            v1.tex_coord = { sprite.get_texture_coords().first.x, sprite.get_texture_coords().first.y };
            v2.tex_coord = { sprite.get_texture_coords().first.x, sprite.get_texture_coords().second.y };
            v3.tex_coord = { sprite.get_texture_coords().second.x, sprite.get_texture_coords().second.y };
            v4.tex_coord = { sprite.get_texture_coords().second.x, sprite.get_texture_coords().first.y };

            prims.push_back(RenderPrim2D({ v1, v2, v3 }));
            prims.push_back(RenderPrim2D({ v1, v3, v4 }));

            auto mat_uid = "internal:game2d/material/sprite_mat_" + sprite.get_id();
            ResourceManager::instance().create_resource(mat_uid,
                    RESOURCE_TYPE_MATERIAL, Material(sprite.get_texture(), { SHADER_COPY_VERT, SHADER_COPY_FRAG }));

            world.pimpl->scene->create_child_object(SPRITE_PREFIX + sprite.get_id(), mat_uid,
                    prims, {});
        }
    }

    static void _render_world(World2D &world) {
        for (auto &kv : world.pimpl->sprites) {
            _render_sprite(world, *kv.second);
        }
    }

    void render_worlds(TimeDelta delta) {
        UNUSED(delta);
        for (auto &kv : g_worlds) {
            auto &world = *kv.second;
            _render_world(world);
        }
    }
}
