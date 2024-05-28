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
#include "internal/game2d/module_game2d.hpp"
#include "internal/game2d/world2d.hpp"
#include "internal/game2d/world2d_layer.hpp"
#include "internal/game2d/pimpl/actor_2d.hpp"
#include "internal/game2d/pimpl/sprite.hpp"
#include "internal/game2d/pimpl/static_object_2d.hpp"
#include "internal/game2d/pimpl/world2d.hpp"
#include "internal/game2d/pimpl/world2d_layer.hpp"

#include <chrono>

#include <cstdint>

#define LAYER_PREFIX "_worldlayer_"

namespace argus {
    World2DLayer::World2DLayer(World2D &world, const std::string &id, uint32_t z_index, float parallax_coeff,
            std::optional<Vector2f> repeat_interval, bool lighting_enabled) :
            pimpl(new pimpl_World2DLayer(world, id, z_index, parallax_coeff, repeat_interval)) {
        auto layer_uuid = Uuid::random().to_string();
        auto layer_id_str = LAYER_PREFIX + world.get_id() + "_" + layer_uuid;
        pimpl->scene = &Scene2D::create(layer_id_str);
        pimpl->scene->set_lighting_enabled(lighting_enabled);
        pimpl->render_camera = &pimpl->scene->create_camera(layer_id_str);
        world.pimpl->canvas.attach_default_viewport_2d(layer_id_str,  *pimpl->render_camera, z_index);
    }

    World2DLayer::~World2DLayer(void) {
        delete pimpl;
    }

    World2D &World2DLayer::get_world(void) const {
        return pimpl->world;
    }

    StaticObject2D &World2DLayer::get_static_object(Handle handle) const {
        auto *obj = g_static_obj_table.deref<StaticObject2D>(handle);
        affirm_precond(obj != nullptr,
                "No such object exists for world layer (in get_static_object)");
        return *obj;
    }

    Handle World2DLayer::create_static_object(const std::string &sprite, const Vector2f &size,
            uint32_t z_index, bool can_occlude_light, const Transform2D &transform) {
        auto *obj = new StaticObject2D(sprite, size, z_index, can_occlude_light, transform);
        pimpl->static_objects.insert(obj->pimpl->handle);
        return obj->pimpl->handle;
    }

    Handle World2DLayer::create_static_object(const std::string &sprite, const Vector2f &size,
            uint32_t z_index, const Transform2D &transform) {
        return create_static_object(sprite, size, z_index, false, transform);
    }

    void World2DLayer::delete_static_object(Handle handle) {
        auto *static_obj = g_static_obj_table.deref<StaticObject2D>(handle);
        affirm_precond(static_obj != nullptr,
                "No such object exists for world layer (in delete_static_object)");

        g_static_obj_table.release_handle(handle);

        pimpl->static_objects.erase(handle);

        auto render_obj = static_obj->pimpl->render_obj;
        if (render_obj.has_value()) {
            pimpl->scene->remove_member_object(render_obj.value());
        }

        delete static_obj;
    }

    Actor2D &World2DLayer::get_actor(Handle handle) const {
        auto *actor = g_actor_table.deref<Actor2D>(handle);
        affirm_precond(actor != nullptr,
                "No such actor exists for world layer (in get_actor)");
        return *actor;
    }

    Handle World2DLayer::create_actor(const std::string &sprite, const Vector2f &size, uint32_t z_index,
            bool can_occlude_light, const Transform2D &transform) {
        UNUSED(can_occlude_light);
        auto *actor = new Actor2D(sprite, size, z_index, can_occlude_light, transform);
        pimpl->actors.insert(actor->pimpl->handle);
        return actor->pimpl->handle;
    }

    Handle World2DLayer::create_actor(const std::string &sprite, const Vector2f &size, uint32_t z_index,
            const Transform2D &transform) {
        return create_actor(sprite, size, z_index, false, transform);
    }

    void World2DLayer::delete_actor(Handle handle) {
        auto *actor = g_actor_table.deref<Actor2D>(handle);
        affirm_precond(actor != nullptr,
                "No such actor exists for world layer (in get_actor)");

        g_actor_table.release_handle(handle);

        pimpl->actors.erase(handle);

        auto render_obj = actor->pimpl->render_obj;
        if (render_obj.has_value()) {
            pimpl->scene->remove_member_object(render_obj.value());
        }

        delete actor;
    }

    static TimeDelta _get_current_frame_duration(const Sprite &sprite) {
        auto &cur_frame = sprite.pimpl->cur_anim->frames[sprite.pimpl->cur_frame.peek()];
        uint64_t dur_ns = uint64_t(cur_frame.duration * sprite.get_animation_speed() * 1'000'000'000.0);
        return std::chrono::nanoseconds(dur_ns);
    }

    Transform2D get_render_transform(const World2DLayer &layer, const Transform2D &world_transform,
            bool include_parallax) {
        auto transform = Transform2D();
        transform.set_translation(world_transform.get_translation()
                * (include_parallax ? layer.pimpl->parallax_coeff : 1.0f)
                / layer.get_world().get_scale_factor());
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

    static Handle _create_render_object(World2DLayer &layer, Sprite &sprite,
            const Vector2f &size, uint32_t z_index, bool can_occlude_light) {
        auto &sprite_def = sprite.pimpl->get_def();

        std::vector<RenderPrim2D> prims;

        auto scaled_size = size / layer.get_world().get_scale_factor();

        Vertex2D v1, v2, v3, v4;

        v1.position = {0, 0};
        v2.position = {0, scaled_size.y};
        v3.position = {scaled_size.x, scaled_size.y};
        v4.position = {scaled_size.x, 0};

        v1.tex_coord = {0, 0};
        v2.tex_coord = {0, 1};
        v3.tex_coord = {1, 1};
        v4.tex_coord = {1, 0};

        auto &anim_tex = ResourceManager::instance().get_resource(sprite_def.atlas);
        auto atlas_w = anim_tex.get<TextureData>().width;
        auto atlas_h = anim_tex.get<TextureData>().height;
        anim_tex.release();

        size_t frame_off = 0;
        for (auto &[_, anim] : sprite.pimpl->get_def().animations) {
            sprite.pimpl->anim_start_offsets.insert({anim.id, frame_off});
            frame_off += anim.frames.size();
        }

        prims.push_back(RenderPrim2D({v1, v2, v3}));
        prims.push_back(RenderPrim2D({v1, v3, v4}));

        //TODO: make this reusable
        auto mat_uid = "internal:game2d/material/sprite_mat_" + Uuid::random().to_string();
        ResourceManager::instance().create_resource(mat_uid, RESOURCE_TYPE_MATERIAL,
                Material(sprite_def.atlas, {}));

        float atlas_stride_x;
        float atlas_stride_y;
        if (sprite.pimpl->get_def().tile_size.x > 0) {
            atlas_stride_x = float(sprite.pimpl->get_def().tile_size.x)
                             / float(atlas_w);
            atlas_stride_y = float(sprite.pimpl->get_def().tile_size.y)
                             / float(atlas_h);
        } else {
            atlas_stride_x = 1.0;
            atlas_stride_y = 1.0;
        }

        return layer.pimpl->scene->create_child_object(mat_uid, prims, scaled_size / 2,
                { atlas_stride_x, atlas_stride_y }, z_index, can_occlude_light ? 1.0 : 0.0, {});
    }

    static void _update_sprite_frame(Sprite &sprite, RenderObject2D &render_obj) {
        auto cur_frame = sprite.pimpl->cur_frame.read();
        if (cur_frame.dirty) {
            render_obj.set_active_frame(sprite.pimpl->cur_anim->frames[cur_frame].offset);
        }
    }

    static void _render_static_object(World2DLayer &layer, StaticObject2D &static_obj) {
        RenderObject2D *render_obj;

        if (static_obj.pimpl->render_obj.has_value()) {
            render_obj = &layer.pimpl->scene->get_object(static_obj.pimpl->render_obj.value()).value().get();
        } else {
            auto handle = _create_render_object(layer, static_obj.get_sprite(),
                    static_obj.get_size(), static_obj.get_z_index(), static_obj.can_occlude_light());
            render_obj = &layer.pimpl->scene->get_object(handle).value().get();
            render_obj->set_transform(get_render_transform(layer, static_obj.get_transform(), false));

            static_obj.pimpl->render_obj = handle;
        }

        _update_sprite_frame(static_obj.get_sprite(), *render_obj);
    }

    static void _render_actor(World2DLayer &layer, Actor2D &actor) {
        RenderObject2D *render_obj;

        if (actor.pimpl->render_obj.has_value()) {
            render_obj = &layer.pimpl->scene->get_object(actor.pimpl->render_obj.value()).value().get();
        } else {
            auto handle = _create_render_object(layer, actor.get_sprite(),
                    actor.get_size(), actor.get_z_index(), actor.pimpl->can_occlude_light.read());
            actor.pimpl->render_obj = handle;

            render_obj = &layer.pimpl->scene->get_object(handle).value().get();
        }

        auto read_occl_light = actor.pimpl->can_occlude_light.read();
        if (read_occl_light.dirty) {
            render_obj->set_light_opacity(read_occl_light ? 1.0 : 0.0);
        }

        auto read_transform = actor.pimpl->transform.read();
        if (read_transform.dirty) {
            render_obj->set_transform(get_render_transform(layer, read_transform.value, false));
        }

        _update_sprite_frame(actor.get_sprite(), *render_obj);
    }

    void render_world_layer(World2DLayer &layer, const ValueAndDirtyFlag<Transform2D> &camera_transform,
        const ValueAndDirtyFlag<float> &al_level, const ValueAndDirtyFlag<Vector3f> &al_color) {
        if (camera_transform.dirty) {
            layer.pimpl->render_camera->set_transform(get_render_transform(layer, camera_transform.value, true));
        }

        if (al_level.dirty) {
            layer.pimpl->scene->set_ambient_light_level(al_level.value);
        }

        if (al_color.dirty) {
            layer.pimpl->scene->set_ambient_light_color(al_color.value);
        }

        for (const auto &obj_handle : layer.pimpl->static_objects) {
            auto *obj = g_static_obj_table.deref<StaticObject2D>(obj_handle);
            if (obj == nullptr) {
                continue;
            }

            if (!obj->get_sprite().is_current_animation_static()) {
                _advance_sprite_animation(obj->get_sprite());
            }
            _render_static_object(layer, *obj);
        }

        for (const auto &actor_handle : layer.pimpl->actors) {
            auto *actor = g_actor_table.deref<Actor2D>(actor_handle);
            if (!actor->get_sprite().is_current_animation_static()) {
                _advance_sprite_animation(actor->get_sprite());
            }
            _render_actor(layer, *actor);
        }
    }
}
