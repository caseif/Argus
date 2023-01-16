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

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/math.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/render/defines.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"
#include "argus/render/common/canvas.hpp"
#include "argus/render/common/material.hpp"
#include "argus/render/common/texture_data.hpp"
#include "argus/render/common/vertex.hpp"

#include "argus/game2d/world2d.hpp"
#include "argus/game2d/world2d_layer.hpp"
#include "internal/game2d/defines.hpp"
#include "internal/game2d/world2d.hpp"
#include "internal/game2d/world2d_layer.hpp"
#include "internal/game2d/pimpl/game_object_2d.hpp"
#include "internal/game2d/pimpl/sprite.hpp"
#include "internal/game2d/pimpl/world2d.hpp"
#include "internal/game2d/pimpl/world2d_layer.hpp"
#include "argus/lowlevel/debug.hpp"

#include <chrono>

#define LAYER_PREFIX "_worldlayer_"
#define GAME_OBJ_PREFIX "_obj_"

namespace argus {
    World2DLayer::World2DLayer(World2D &world, const std::string &id, float parallax_coeff,
            std::optional<Vector2f> repeat_interval):
        pimpl(new pimpl_World2DLayer(world, id, parallax_coeff, repeat_interval)) {
        auto layer_uuid = Uuid::random().to_string();
        auto layer_id_str = LAYER_PREFIX + world.get_id() + "_" + layer_uuid;
        pimpl->scene = &Scene2D::create(layer_id_str);
        pimpl->render_camera = &pimpl->scene->create_camera(layer_id_str);
        world.pimpl->canvas.attach_default_viewport_2d(layer_id_str, *pimpl->render_camera);
    }

    World2DLayer::~World2DLayer(void) {
        delete pimpl;
    }

    World2D &World2DLayer::get_world(void) const {
        return pimpl->world;
    }

    GameObject2D &World2DLayer::get_object(const Uuid &uuid) const {
        auto it = pimpl->objects.find(uuid);
        _ARGUS_ASSERT(it != pimpl->objects.cend(), "No object with UUID exists for world layer (in get_object)");
        return *it->second;
    }

    GameObject2D &World2DLayer::create_object(const std::string &sprite, const Vector2f &size,
            const Transform2D &transform) {
        auto *obj = new GameObject2D(sprite, size, transform);
        pimpl->objects.insert({ obj->get_uuid(), obj });
        return *obj;
    }

    void World2DLayer::delete_object(const Uuid &uuid) {
        auto it = pimpl->objects.find(uuid);
        _ARGUS_ASSERT(it != pimpl->objects.cend(), "No object with UUID exists for world layer (in delete_object)");

        pimpl->objects.erase(it);
    }

    static TimeDelta _get_current_frame_duration(const Sprite &sprite) {
        auto &cur_frame = sprite.pimpl->cur_anim->frames[sprite.pimpl->cur_frame.peek()];
        uint64_t dur_ns = static_cast<uint64_t>(cur_frame.duration * sprite.get_animation_speed() * 1'000'000'000.0);
        return std::chrono::nanoseconds(dur_ns);
    }

    Transform2D get_render_transform(const World2D &world, const Transform2D &world_transform) {
        auto transform = Transform2D();
        transform.set_translation(world_transform.get_translation() / world.get_scale_factor());
        transform.set_rotation(world_transform.get_rotation());
        transform.set_scale(world_transform.get_scale());
        return transform;
    }

    static void _advance_sprite_animation(Sprite &sprite) {
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

    static void _render_object(World2DLayer &layer, GameObject2D &game_obj) {
        RenderObject2D *render_obj;

        auto &sprite = game_obj.get_sprite();

        auto render_obj_opt = layer.pimpl->scene->get_object(GAME_OBJ_PREFIX + game_obj.get_uuid().to_string());
        if (render_obj_opt.has_value()) {
            render_obj = &render_obj_opt->get();
        } else {
            auto &sprite_def = sprite.pimpl->get_def();

            std::vector<RenderPrim2D> prims;

            auto scaled_size = game_obj.get_size() / layer.get_world().get_scale_factor();

            Vertex2D v1, v2, v3, v4;

            v1.position = { 0, 0 };
            v2.position = { 0, scaled_size.y };
            v3.position = { scaled_size.x, scaled_size.y };
            v4.position = { scaled_size.x, 0 };

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

            //TODO: make this reusable
            auto mat_uid = "internal:game2d/material/sprite_mat_" + game_obj.get_uuid().to_string();
            ResourceManager::instance().create_resource(mat_uid, RESOURCE_TYPE_MATERIAL,
                    Material(sprite_def.atlas, { SHADER_SPRITE_VERT, SHADER_SPRITE_FRAG }));

            float atlas_stride_x;
            float atlas_stride_y;
            if (sprite.pimpl->get_def().tile_size.x > 0) {
                atlas_stride_x = static_cast<float>(sprite.pimpl->get_def().tile_size.x)
                                       / static_cast<float>(atlas_w);
                atlas_stride_y = static_cast<float>(sprite.pimpl->get_def().tile_size.y)
                                       / static_cast<float>(atlas_h);
            } else {
                atlas_stride_x = 1.0;
                atlas_stride_y = 1.0;
            }

            render_obj = &layer.pimpl->scene->create_child_object(GAME_OBJ_PREFIX + game_obj.get_uuid().to_string(),
                    mat_uid, prims, { atlas_stride_x, atlas_stride_y }, {});
        }

        auto read_transform = game_obj.pimpl->transform.read();
        if (read_transform.dirty) {
            render_obj->set_transform(get_render_transform(layer.get_world(), read_transform));
        }

        auto cur_frame = sprite.pimpl->cur_frame.read();
        if (cur_frame.dirty) {
            render_obj->set_active_frame(sprite.pimpl->cur_anim->frames[cur_frame].offset);
        }
    }

    void render_world_layer(World2DLayer &layer) {
        auto camera_transform = layer.get_world().pimpl->abstract_camera.read();
        if (camera_transform.dirty) {
            layer.pimpl->render_camera->set_transform(get_render_transform(layer.get_world(), camera_transform));
        }

        for (auto &kv : layer.pimpl->objects) {
            if (!kv.second->get_sprite().is_current_animation_static()) {
                _advance_sprite_animation(kv.second->get_sprite());
            }
            _render_object(layer, *kv.second);
        }
    }
}
