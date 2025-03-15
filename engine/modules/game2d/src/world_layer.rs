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

use crate::actor::Actor2d;
use std::collections::HashMap;
use std::ops::DerefMut;
use std::time::Duration;
use uuid::Uuid;
use argus_render::common::{Material, RenderCanvas, TextureData, Transform2d, Vertex2d};
use argus_render::constants::RESOURCE_TYPE_MATERIAL;
use argus_render::twod::{get_render_context_2d, Light2dProperties, Light2dType, RenderPrimitive2d, Scene2d};
use argus_resman::ResourceManager;
use argus_scripting_bind::script_bind;
use argus_util::dirtiable::ValueAndDirtyFlag;
use argus_util::math::{Vector2f, Vector3f};
use argus_util::pool::Handle;
use crate::sprite::Sprite;
use crate::static_object::StaticObject2d;

const LAYER_PREFIX: &str = "_worldlayer_";

#[script_bind(ref_only)]
pub struct World2dLayer {
    id: String,
    world_id: String,

    z_index: u32,
    parallax_coeff: f32,
    repeat_interval: Option<Vector2f>,

    scene_id: String,
    render_camera_id: String,

    static_objects: HashMap<Uuid, StaticObject2d>,
    actors: HashMap<Uuid, Actor2d>,
}

#[script_bind]
impl World2dLayer {
    pub(crate) fn new(
        world_id: String,
        id: String,
        z_index: u32,
        parallax_coeff: f32,
        repeat_interval: Option<Vector2f>,
        lighting_enabled: bool,
        canvas: &mut RenderCanvas,
    ) -> World2dLayer {
        let layer_uuid = Uuid::new_v4().to_string();
        let layer_id_str = format!("{}{}_{}", LAYER_PREFIX, world_id, layer_uuid);

        let mut scene = get_render_context_2d()
            .create_scene(layer_id_str.as_str(), Transform2d::default());
        scene.set_lighting_enabled(lighting_enabled);
        scene.add_light(
            Light2dProperties {
                ty: Light2dType::Point,
                is_occludable: true,
                color: Vector3f::new(1.0, 0.0, 1.0),
                intensity: 1.0,
                falloff_gradient: 1,
                falloff_multiplier: 5.0,
                falloff_buffer: 0.5,
                shadow_falloff_gradient: 2,
                shadow_falloff_multiplier: 3.0,
            },
            Transform2d::new(Vector2f::new(0.25, 0.5), Vector2f::new(1.0, 1.0), 0.0),
        );
        scene.add_light(
            Light2dProperties {
                ty: Light2dType::Point,
                color: Vector3f::new(0.0, 1.0, 1.0),
                is_occludable: true,
                intensity: 1.0,
                falloff_gradient: 1,
                falloff_multiplier: 5.0,
                falloff_buffer: 0.5,
                shadow_falloff_gradient: 2,
                shadow_falloff_multiplier: 3.0,
            },
            Transform2d::new(Vector2f::new(0.75, 0.5), Vector2f::new(1.0, 1.0), 0.0),
        );

        let scene_id = scene.get_id().to_string();
        let render_camera = scene.create_camera(layer_id_str.as_str());
        canvas.add_default_viewport_2d(
            layer_id_str.as_str(),
            scene_id,
            render_camera.get_id(),
            z_index
        ).expect("Failed to create viewport");

        let layer = World2dLayer {
            world_id,
            id,
            z_index,
            parallax_coeff,
            repeat_interval,
            scene_id: layer_id_str.clone(),
            render_camera_id: layer_id_str,
            static_objects: Default::default(),
            actors: Default::default(),
        };

        layer
    }

    fn get_scene_id(&self) -> &str {
        self.scene_id.as_str()
    }

    fn get_render_camera_id(&self) -> &str {
        self.render_camera_id.as_str()
    }

    #[script_bind]
    pub fn get_world_id(&self) -> &str {
        self.world_id.as_str()
    }

    pub fn get_static_object(&self, id: &Uuid) -> Result<&StaticObject2d, &'static str> {
        match self.static_objects.get(&id) {
            Some(obj) => Ok(obj),
            None => Err("No such object exists for world layer (in get_static_object)"),
        }
    }

    pub fn get_static_object_mut(&mut self, id: &Uuid) -> Result<&mut StaticObject2d, &'static str> {
        match self.static_objects.get_mut(&id) {
            Some(obj) => Ok(obj),
            None => Err("No such object exists for world layer (in get_static_object)"),
        }
    }

    pub fn create_static_object(
        &mut self,
        sprite: &str,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
        transform: Transform2d,
    ) -> Result<Uuid, String> {
        let sprite_defn_res = match ResourceManager::instance().get_resource(sprite) {
            Ok(res) => res,
            Err(e) => { return Err(e.info); }
        };

        let obj = StaticObject2d::new(sprite_defn_res, size, z_index, can_occlude_light, transform);
        let id = Uuid::new_v4();
        self.static_objects.insert(id, obj);
        Ok(id)
    }

    pub fn delete_static_object(&mut self, id: &Uuid) -> Result<(), &'static str> {
        match self.static_objects.remove(&id) {
            Some(obj) => {
                if let Some(render_obj) = obj.render_obj {
                    let context = get_render_context_2d();
                    context.get_scene_mut(self.get_scene_id()).unwrap().remove_object(render_obj);
                }
                Ok(())
            }
            None => Err("No such object exists for world layer (in delete_static_object)"),
        }
    }

    pub fn get_actor(&self, id: &Uuid) -> Result<&Actor2d, &'static str> {
        match self.actors.get(id) {
            Some(actor) => Ok(actor),
            None => Err("No such actor exists for world layer (in get_actor)"),
        }
    }

    pub fn get_actor_mut(&mut self, id: &Uuid) -> Result<&mut Actor2d, &'static str> {
        match self.actors.get_mut(id) {
            Some(actor) => Ok(actor),
            None => Err("No such actor exists for world layer (in get_actor_mut)")
        }
    }

    pub fn create_actor(
        &mut self,
        sprite_uid: String,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
    ) -> Result<Uuid, String> {
        let sprite_defn_res = match ResourceManager::instance().get_resource(&sprite_uid) {
            Ok(res) => res,
            Err(e) => { return Err(e.info); }
        };

        let actor = Actor2d::new(sprite_defn_res, size, z_index, can_occlude_light);
        let id = Uuid::new_v4();
        self.actors.insert(id, actor);
        Ok(id)
    }

    pub fn delete_actor(&mut self, id: &Uuid) -> Result<(), &'static str> {
        match self.actors.remove(id) {
            Some(actor) => {
                if let Some(render_obj) = actor.render_obj {
                    let context = get_render_context_2d();
                    context.get_scene_mut(self.get_scene_id()).unwrap().remove_object(render_obj);
                }

                Ok(())
            }
            None => Err("No such actor exists for world layer (in get_actor)"),
        }
    }

    pub(crate) fn simulate(&mut self, delta: Duration) {
        for (_, actor) in &mut self.actors {
            if actor.velocity.x != 0.0 || actor.velocity.y != 0.0 {
                let mut new_transform = actor.transform.peek().value;
                new_transform.translation += actor.velocity * delta.as_secs_f32();
                actor.transform.set(new_transform);
            }
        }
    }

    fn get_render_transform(
        &self,
        world_transform: &Transform2d,
        world_scale_factor: f32,
        include_parallax: bool,
    ) -> Transform2d {
        let parallax_coeff = if include_parallax {
            self.parallax_coeff
        } else {
            1.0
        };

        Transform2d::new(
            world_transform.translation * parallax_coeff / world_scale_factor,
            world_transform.scale,
            world_transform.rotation,
        )
    }

    fn create_render_object(
        world_id: String,
        scene: &mut Scene2d,
        sprite: &mut Sprite,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
        scale_factor: f32,
    ) -> Handle {
        let sprite_def = sprite.get_definition().clone();

        let mut prims: Vec<RenderPrimitive2d> = Vec::new();

        let scaled_size = size / scale_factor;

        let v1 = Vertex2d {
            position: Vector2f::new(0.0, 0.0),
            tex_coord: Vector2f::new(0.0, 0.0),
            normal: Default::default(),
            color: Default::default(),
        };
        let v2 = Vertex2d {
            position: Vector2f::new(0.0, scaled_size.y),
            tex_coord: Vector2f::new(0.0, 1.0),
            normal: Default::default(),
            color: Default::default(),
        };
        let v3 = Vertex2d {
            position: Vector2f::new(scaled_size.x, scaled_size.y),
            tex_coord: Vector2f::new(1.0, 1.0),
            normal: Default::default(),
            color: Default::default(),
        };
        let v4 = Vertex2d {
            position: Vector2f::new(scaled_size.x, 0.0),
            tex_coord: Vector2f::new(1.0, 0.0),
            normal: Default::default(),
            color: Default::default(),
        };

        let anim_tex_res = ResourceManager::instance()
            .get_resource(&sprite_def.atlas)
            .expect("Failed to load sprite atlas");
        let anim_tex = anim_tex_res.get::<TextureData>().unwrap();
        let atlas_w = anim_tex.get_width();
        let atlas_h = anim_tex.get_height();

        let mut frame_off = 0;
        for anim in sprite_def.animations.values() {
            sprite.get_animation_start_offsets_mut().insert(anim.get_id().to_string(), frame_off);
            frame_off += anim.frames.len();
        }

        prims.push(RenderPrimitive2d::new(vec![v1, v2, v3]));
        prims.push(RenderPrimitive2d::new(vec![v1, v3, v4]));

        //TODO: make this reusable
        let mat_uid = format!("internal:game2d/material/sprite_mat_{}", Uuid::new_v4().to_string());
        let mat = Material::new(sprite_def.atlas.clone(), vec![]);
        let mat_resource = ResourceManager::instance()
            .create_resource(
                mat_uid.as_str(),
                RESOURCE_TYPE_MATERIAL,
                Box::new(mat),
            )
            .expect("Failed to create material resource");

        let (atlas_stride_x, atlas_stride_y) = if sprite.get_definition().tile_size.x > 0 {
            (
                (sprite.get_definition().tile_size.x as f32) / (atlas_w as f32),
                (sprite.get_definition().tile_size.y as f32) / (atlas_h as f32),
            )
        } else {
            (1.0, 1.0)
        };

        scene.add_object(
            mat_resource,
            prims,
            scaled_size / 2.0,
            Vector2f::new(atlas_stride_x, atlas_stride_y),
            z_index,
            if can_occlude_light { 1.0 } else { 0.0 },
            Transform2d::default(),
        )
    }

    fn render_static_object(&mut self, scene: &mut Scene2d, id: &Uuid, scale_factor: f32) {
        let static_obj = self.static_objects.get(id).expect("Static object was missing");
        let mut render_obj = match static_obj.render_obj {
            Some(obj_id) => {
                scene.get_object_mut(obj_id).expect("Render object was missing for static object")
            }
            None => {
                let world_id = self.get_world_id().to_string();
                let size = static_obj.get_size();
                let z_index = static_obj.get_z_index();
                let can_occlude_light = static_obj.get_can_occlude_light();
                let render_transform = self.get_render_transform(
                    &static_obj.get_transform(),
                    scale_factor,
                    false
                );
                // upgrade reference
                let static_obj = self.static_objects.get_mut(id)
                    .expect("Static object was missing");
                let sprite = static_obj.get_sprite_mut();

                let new_obj_handle = Self::create_render_object(
                    world_id,
                    scene,
                    sprite,
                    size,
                    z_index,
                    can_occlude_light,
                    scale_factor,
                );
                static_obj.render_obj = Some(new_obj_handle);

                let mut new_obj = scene.get_object_mut(new_obj_handle)
                    .expect("Render object was missing for static object");
                new_obj.set_transform(render_transform.into());

                new_obj
            }
        };

        // upgrade object reference to mutable
        let static_obj = self.static_objects.get_mut(id).expect("Static object was missing");
        static_obj.get_sprite_mut().update_current_frame(&mut render_obj);
    }

    fn render_actor(&mut self, scene: &mut Scene2d, id: &Uuid, scale_factor: f32) {
        let actor = self.actors.get(id).expect("Actor was missing");

        let mut render_obj = match actor.render_obj {
            Some(obj_handle) => scene.get_object_mut(obj_handle).unwrap(),
            None => {
                let world_id = self.get_world_id().to_string();
                let size = actor.get_size();
                let z_index = actor.get_z_index();
                let render_transform = self.get_render_transform(
                    &actor.get_transform(),
                    scale_factor,
                    false
                );
                // upgrade reference
                let actor = self.actors.get_mut(id).expect("Actor was missing");
                let can_occlude_light = actor.can_occlude_light.read().value;
                let sprite = actor.get_sprite_mut();

                let new_obj_handle = Self::create_render_object(
                    world_id,
                    scene,
                    sprite,
                    size,
                    z_index,
                    can_occlude_light,
                    scale_factor,
                );
                actor.render_obj = Some(new_obj_handle);

                let mut new_obj = scene.get_object_mut(new_obj_handle)
                    .expect("Render object was missing for static object");
                new_obj.set_transform(render_transform.into());

                new_obj
            }
        };

        let read_occl_light;
        let read_transform;
        {
            // upgrade reference so we can reset the dirty flags
            let actor = self.actors.get_mut(id).expect("Actor was missing");
            read_occl_light = actor.can_occlude_light.read();
            read_transform = actor.transform.read();
        }

        if read_occl_light.dirty {
            render_obj.set_light_opacity(if read_occl_light.value { 1.0 } else { 0.0 });
        }

        if read_transform.dirty {
            render_obj.set_transform(self
                .get_render_transform(
                    &read_transform.value,
                    scale_factor,
                    false
                ).into());
        }

        // upgrade reference
        let actor = self.actors.get_mut(id).expect("Actor was missing");
        actor.get_sprite_mut().update_current_frame(&mut render_obj);
    }

    pub(crate) fn render(
        &mut self,
        scale_factor: f32,
        camera_transform: &ValueAndDirtyFlag<Transform2d>,
        al_level: &ValueAndDirtyFlag<f32>,
        al_color: &ValueAndDirtyFlag<Vector3f>,
    ) {
        let context = get_render_context_2d();

        if camera_transform.dirty {
            context
                .get_scene_mut(&self.scene_id)
                .unwrap()
                .get_camera_mut(self.get_render_camera_id())
                .unwrap()
                .set_transform(
                    self.get_render_transform(
                        &camera_transform.value,
                        scale_factor,
                        true,
                    )
                );
        }

        let mut scene = context.get_scene_mut(self.get_scene_id()).unwrap();

        if al_level.dirty {
            scene.set_ambient_light_level(al_level.value);
        }

        if al_color.dirty {
            scene.set_ambient_light_color(al_color.value);
        }

        for obj in self.static_objects.values_mut() {
            if !obj.get_sprite().is_current_animation_static() {
                obj.get_sprite_mut().advance_animation();
            }
        }
        let objs_to_render = self.static_objects.keys().cloned().collect::<Vec<_>>();
        for obj_id in objs_to_render {
            self.render_static_object(scene.deref_mut(), &obj_id, scale_factor);
        }

        for actor in self.actors.values_mut() {
            if !actor.get_sprite().is_current_animation_static() {
                actor.get_sprite_mut().advance_animation();
            }
        }
        let actors_to_render = self.actors.keys().cloned().collect::<Vec<_>>();
        for actor_id in actors_to_render {
            self.render_actor(scene.deref_mut(), &actor_id, scale_factor);
        }
    }
}
