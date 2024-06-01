/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/scripting.hpp"

#include "argus/render/common/canvas.hpp"

#include "argus/game2d/world2d.hpp"
#include "internal/game2d/script_bindings.hpp"

namespace argus {
    static void _bind_world_symbols(void) {
        bind_type<World2D>("World2D");
        bind_member_static_function<World2D>("create", &World2D::create);
        bind_member_static_function<World2D>("get", &World2D::get);
        bind_member_instance_function("get_id", &World2D::get_id);
        bind_member_instance_function("get_scale_factor", &World2D::get_scale_factor);
        bind_member_instance_function("get_camera_transform", &World2D::get_camera_transform);
        bind_member_instance_function("set_camera_transform", &World2D::set_camera_transform);
        bind_member_instance_function("get_ambient_light_level", &World2D::get_ambient_light_level);
        bind_member_instance_function("set_ambient_light_level", &World2D::set_ambient_light_level);
        bind_member_instance_function("get_ambient_light_color", &World2D::get_ambient_light_color);
        bind_member_instance_function("set_ambient_light_color", &World2D::set_ambient_light_color);
        bind_member_instance_function("get_background_layer", &World2D::get_background_layer);
        bind_extension_function<World2D>(
                "add_background_layer",
                +[](World2D &world, float parallax) -> World2DLayer & {
                    return world.add_background_layer(parallax, std::nullopt);
                }
        );
        bind_extension_function<World2D>(
                "add_repeating_background_layer",
                +[](World2D &world, float parallax, float interval_x, float interval_y) -> World2DLayer & {
                    return world.add_background_layer(parallax, Vector2f { interval_x, interval_y });
                }
        );
        bind_member_instance_function("get_static_object", &World2D::get_static_object);
        bind_member_instance_function("create_static_object", &World2D::create_static_object);
        bind_member_instance_function("delete_static_object", &World2D::delete_static_object);
        bind_member_instance_function("get_actor", &World2D::get_actor);
        bind_member_instance_function("create_actor", &World2D::create_actor);
        bind_member_instance_function("delete_actor", &World2D::delete_actor);
    }

    static void _bind_world_layer_symbols(void) {
        bind_type<World2DLayer>("World2DLayer");
        bind_member_instance_function("get_static_object", &World2DLayer::get_static_object);
        bind_member_instance_function("get_world", &World2DLayer::get_world);
        bind_member_instance_function<Handle(World2DLayer::*)(const std::string &, const Vector2f &, uint32_t,
                const Transform2D &)>("create_static_object", &World2DLayer::create_static_object);
        bind_member_instance_function("delete_static_object", &World2DLayer::delete_static_object);
        bind_member_instance_function("get_actor", &World2DLayer::get_actor);
        bind_member_instance_function<Handle(World2DLayer::*)(const std::string &, const Vector2f &, uint32_t,
                const Transform2D &)>("create_actor", &World2DLayer::create_actor);
        bind_member_instance_function("delete_actor", &World2DLayer::delete_actor);
    }

    static void _bind_sprite_symbols(void) {
        bind_type<Sprite>("Sprite");
        bind_member_instance_function("get_animation_speed", &Sprite::get_animation_speed);
        bind_member_instance_function("set_animation_speed", &Sprite::set_animation_speed);
        bind_member_instance_function("get_available_animations", &Sprite::get_available_animations);
        bind_member_instance_function("get_current_animation", &Sprite::get_current_animation);
        bind_member_instance_function("set_current_animation", &Sprite::set_current_animation);
        bind_member_instance_function("does_current_animation_loop", &Sprite::does_current_animation_loop);
        bind_member_instance_function("is_current_animation_static", &Sprite::is_current_animation_static);
        bind_member_instance_function("get_current_animation_padding", &Sprite::get_current_animation_padding);
        bind_member_instance_function("pause_animation", &Sprite::pause_animation);
        bind_member_instance_function("resume_animation", &Sprite::resume_animation);
        bind_member_instance_function("reset_animation", &Sprite::reset_animation);
    }

    static void _bind_static_object_symbols(void) {
        bind_type<StaticObject2D>("StaticObject2D");
        bind_member_instance_function("get_size", &StaticObject2D::get_size);
        bind_member_instance_function("get_z_index", &StaticObject2D::get_z_index);
        bind_member_instance_function("get_transform", &StaticObject2D::get_transform);
        bind_member_instance_function("get_sprite", &StaticObject2D::get_sprite);
    }

    static void _bind_actor_symbols(void) {
        bind_type<Actor2D>("Actor2D");
        bind_member_instance_function("get_size", &Actor2D::get_size);
        bind_member_instance_function("get_z_index", &Actor2D::get_z_index);
        bind_member_instance_function("get_transform", &Actor2D::get_transform);
        bind_member_instance_function("set_transform", &Actor2D::set_transform);
        bind_member_instance_function("get_sprite", &Actor2D::get_sprite);
    }

    void register_game2d_bindings(void) {
        _bind_world_symbols();
        _bind_world_layer_symbols();
        _bind_static_object_symbols();
        _bind_sprite_symbols();
        _bind_actor_symbols();
    }
}
