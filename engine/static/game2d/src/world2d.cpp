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
#include "internal/game2d/pimpl/animated_sprite.hpp"
#include "internal/game2d/pimpl/sprite.hpp"
#include "internal/game2d/pimpl/world2d.hpp"
#include "internal/game2d/defines.hpp"

#include <map>
#include <string>
#include <utility>

#define WORLD_PREFIX "_world_"
#define SPRITE_PREFIX "_sprite_"
#define ANIM_SPRITE_PREFIX "_asprite_"

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_World2D));

    static std::map<std::string, World2D*> g_worlds;

    World2D &World2D::create(const std::string &id, Canvas &canvas) {
        auto *world = new World2D(id, canvas);
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

    static TimeDelta _get_current_frame_duration(const AnimatedSprite &sprite) {
        auto &cur_frame = sprite.pimpl->cur_anim->frames[sprite.pimpl->cur_frame.peek()];
        uint64_t dur_ns = static_cast<uint64_t>(cur_frame.duration * sprite.get_animation_speed() * 1'000'000'000.0);
        return std::chrono::nanoseconds(dur_ns);
    }

    AnimatedSprite &World2D::add_animated_sprite(const std::string &id, const std::string &sprite_uid) {
        if (pimpl->anim_sprites.find(id) != pimpl->anim_sprites.cend()) {
            throw std::invalid_argument("Animated sprite with given ID already exists in the world");
        }

        auto &def_res = ResourceManager::instance().get_resource(sprite_uid);

        auto *sprite = new AnimatedSprite(id, def_res);

        sprite->pimpl->next_frame_update = now() + _get_current_frame_duration(*sprite);

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
        RenderObject2D *obj;

        auto obj_opt = world.pimpl->scene->get_object(SPRITE_PREFIX + sprite.get_id());
        if (obj_opt.has_value()) {
            obj = &obj_opt->get();
        } else {
            std::vector<RenderPrim2D> prims;

            Vertex2D v1, v2, v3, v4;

            v1.position = { 0, 0 };
            v2.position = { 0, sprite.get_base_size().y };
            v3.position = { sprite.get_base_size().x, sprite.get_base_size().y };
            v4.position = { sprite.get_base_size().x, 0 };

            auto &tc1 = sprite.get_texture_coords().first;
            auto &tc2 = sprite.get_texture_coords().second;

            v1.tex_coord = { tc1.x, tc1.y };
            v2.tex_coord = { tc1.x, tc2.y };
            v3.tex_coord = { tc2.x, tc2.y };
            v4.tex_coord = { tc2.x, tc1.y };

            prims.push_back(RenderPrim2D({ v1, v2, v3 }));
            prims.push_back(RenderPrim2D({ v1, v3, v4 }));

            auto mat_uid = "internal:game2d/material/sprite_mat_" + sprite.get_id();
            ResourceManager::instance().create_resource(mat_uid, RESOURCE_TYPE_MATERIAL,
                    Material(sprite.get_texture(), { SHADER_STATIC_SPRITE_VERT, SHADER_STATIC_SPRITE_FRAG }));

            obj = &world.pimpl->scene->create_child_object(SPRITE_PREFIX + sprite.get_id(), mat_uid,
                    prims, {}, {});
        }

        if (sprite.pimpl->transform_dirty) {
            obj->set_transform(sprite.pimpl->transform);
        }
    }

    static void _advance_sprite_animation(AnimatedSprite &sprite) {
        if (sprite.pimpl->pending_reset) {
            sprite.pimpl->cur_frame = 0;
            sprite.pimpl->next_frame_update = now() + _get_current_frame_duration(sprite);
            return;
        }

        if (sprite.pimpl->paused) {
            //TODO: we don't update next_frame_update so the next frame would
            //      likely be displayed immediately after unpausing - we should
            //      store the remaining time instead
            return;
        }

        auto &anim = sprite.pimpl->cur_anim;
        // you could theoretically force an infinite loop if now() was evaluated every iteration
        auto cur_time = now();
        while (cur_time >= sprite.pimpl->next_frame_update) {
            if (sprite.pimpl->cur_frame.peek() == anim->frames.size() - 1) {
                sprite.pimpl->cur_frame = 0;
            } else {
                sprite.pimpl->cur_frame += 1;
            }

            auto frame_dur = _get_current_frame_duration(sprite);
            sprite.pimpl->next_frame_update += frame_dur;
        }
    }

    static void _render_animated_sprite(World2D &world, AnimatedSprite &sprite) {
        RenderObject2D *obj;

        auto obj_opt = world.pimpl->scene->get_object(ANIM_SPRITE_PREFIX + sprite.get_id());
        if (obj_opt.has_value()) {
            obj = &obj_opt->get();
        } else {
            auto &sprite_def = sprite.pimpl->get_def();

            std::vector<RenderPrim2D> prims;

            Vertex2D v1, v2, v3, v4;

            v1.position = { 0, 0 };
            v2.position = { 0, sprite.get_base_size().y };
            v3.position = { sprite.get_base_size().x, sprite.get_base_size().y };
            v4.position = { sprite.get_base_size().x, 0 };

            v1.tex_coord = { 0, 0 };
            v2.tex_coord = { 0, 1 };
            v3.tex_coord = { 1, 1 };
            v4.tex_coord = { 1, 0 };

            auto &anim_tex = ResourceManager::instance().get_resource(sprite_def.atlas);
            auto atlas_w = anim_tex.get<TextureData>().width;
            auto atlas_h = anim_tex.get<TextureData>().height;
            anim_tex.release();

            size_t frame_off = 0;
            for (auto &kv : sprite.pimpl->get_def().animations) {
                auto &anim = kv.second;

                sprite.pimpl->anim_start_offsets.insert({ anim.id, frame_off });
                frame_off += anim.frames.size();
            }

            prims.push_back(RenderPrim2D({ v1, v2, v3 }));
            prims.push_back(RenderPrim2D({ v1, v3, v4 }));

            auto mat_uid = "internal:game2d/material/anim_sprite_mat_" + sprite.get_id();
            ResourceManager::instance().create_resource(mat_uid, RESOURCE_TYPE_MATERIAL,
                    Material(sprite_def.atlas, { SHADER_ANIM_SPRITE_VERT, SHADER_ANIM_SPRITE_FRAG }));

            float atlas_stride_x = static_cast<float>(sprite.pimpl->get_def().tile_size.x)
                    / static_cast<float>(atlas_w);
            float atlas_stride_y = static_cast<float>(sprite.pimpl->get_def().tile_size.y)
                    / static_cast<float>(atlas_h);

            obj = &world.pimpl->scene->create_child_object(ANIM_SPRITE_PREFIX + sprite.get_id(), mat_uid,
                    prims, { atlas_stride_x, atlas_stride_y }, {});
        }

        if (sprite.pimpl->transform_dirty) {
            obj->set_transform(sprite.pimpl->transform);
        }

        auto cur_frame = sprite.pimpl->cur_frame.read();
        if (cur_frame.dirty) {
            obj->set_active_frame(sprite.pimpl->cur_anim->frames[cur_frame].offset);
        }
    }

    static void _render_world(World2D &world) {
        for (auto &kv : world.pimpl->sprites) {
            _render_sprite(world, *kv.second);
        }

        for (auto &kv : world.pimpl->anim_sprites) {
            _advance_sprite_animation(*kv.second);
            _render_animated_sprite(world, *kv.second);
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
