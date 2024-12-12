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
use lowlevel_rustabi::argus::lowlevel::{FfiWrapper, Handle, ValueAndDirtyFlag, Vector2f, Vector3f};
use render_rustabi::argus::render::*;
use resman_rustabi::argus::resman::ResourceManager;
use std::collections::HashMap;
use std::slice;
use uuid::Uuid;
use argus_scripting_bind::script_bind;
use render_rustabi::render_cabi::argus_material_len;
use crate::sprite::Sprite;
use crate::static_object::StaticObject2d;

const LAYER_PREFIX: &str = "_worldlayer_";

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

impl World2dLayer {
    pub(crate) fn new(
        world_id: String,
        id: String,
        z_index: u32,
        parallax_coeff: f32,
        repeat_interval: Option<Vector2f>,
        lighting_enabled: bool,
        canvas: &mut Canvas,
    ) -> World2dLayer {
        let layer_uuid = Uuid::new_v4().to_string();
        let layer_id_str = format!("{}{}_{}", LAYER_PREFIX, world_id, layer_uuid);

        let mut scene = Scene2d::create(layer_id_str.as_str());
        scene.set_lighting_enabled(lighting_enabled);
        scene.add_light(
            Light2dType::Point,
            true,
            Vector3f::new(1.0, 0.0, 1.0),
            Light2dParameters {
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
            Light2dType::Point,
            true,
            Vector3f::new(0.0, 1.0, 1.0),
            Light2dParameters {
                intensity: 1.0,
                falloff_gradient: 1,
                falloff_multiplier: 5.0,
                falloff_buffer: 0.5,
                shadow_falloff_gradient: 2,
                shadow_falloff_multiplier: 3.0,
            },
            Transform2d::new(Vector2f::new(0.75, 0.5), Vector2f::new(1.0, 1.0), 0.0),
        );

        let render_camera = scene.create_camera(layer_id_str.as_str());
        canvas.attach_default_viewport_2d(layer_id_str.as_str(), &render_camera, z_index);

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

    fn get_scene(&self) -> Scene2d {
        let scene = Scene::find(&self.scene_id).expect("Scene for world layer was missing");
        assert_eq!(scene.get_type(), SceneType::TwoD);
        scene.as_2d()
    }

    fn get_render_camera(&self) -> Camera2d {
        self.get_scene().find_camera(self.render_camera_id.as_str())
    }

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
        let sprite_defn_res = match ResourceManager::get_instance().get_resource(sprite) {
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
                    self.get_scene().remove_object(render_obj);
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
        transform: Transform2d,
    ) -> Result<Uuid, String> {
        let sprite_defn_res = match ResourceManager::get_instance().get_resource(&sprite_uid) {
            Ok(res) => res,
            Err(e) => { return Err(e.info); }
        };

        let actor = Actor2d::new(sprite_defn_res, size, z_index, can_occlude_light, transform);
        let id = Uuid::new_v4();
        self.actors.insert(id, actor);
        Ok(id)
    }

    pub fn delete_actor(&mut self, id: &Uuid) -> Result<(), &'static str> {
        match self.actors.remove(id) {
            Some(actor) => {
                if let Some(render_obj) = actor.render_obj {
                    self.get_scene().remove_object(render_obj);
                }

                Ok(())
            }
            None => Err("No such actor exists for world layer (in get_actor)"),
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

        let anim_tex = ResourceManager::get_instance()
            .get_resource(&sprite_def.atlas)
            .expect(format!("Failed to load sprite atlas '{}'", sprite_def.atlas).as_str());
        let atlas_w = anim_tex.get::<TextureData>().get_width();
        let atlas_h = anim_tex.get::<TextureData>().get_height();
        anim_tex.release();

        let mut frame_off = 0;
        for anim in sprite_def.animations.values() {
            sprite.get_animation_start_offsets_mut().insert(anim.get_id().to_string(), frame_off);
            frame_off += anim.frames.len();
        }

        prims.push(RenderPrimitive2d::new(vec![v1, v2, v3]));
        prims.push(RenderPrimitive2d::new(vec![v1, v3, v4]));

        //TODO: make this reusable
        let mat_uid = format!("internal:game2d/material/sprite_mat_{}", Uuid::new_v4().to_string());
        let mut mat = Material::new(sprite_def.atlas.clone(), vec![]);
        ResourceManager::get_instance()
            .create_resource(
                mat_uid.as_str(),
                RESOURCE_TYPE_MATERIAL,
                unsafe { slice::from_raw_parts(mat.handle.cast(), argus_material_len()) },
            )
            .expect("Failed to create material resource");
        // leak the material so it stays valid after this function finishes executing
        // MaterialLoader will implicitly delete it when the resource is deleted
        unsafe { mat.leak(); }

        let (atlas_stride_x, atlas_stride_y) = if sprite.get_definition().tile_size.x > 0 {
            (
                (sprite.get_definition().tile_size.x as f32) / (atlas_w as f32),
                (sprite.get_definition().tile_size.y as f32) / (atlas_h as f32),
            )
        } else {
            (1.0, 1.0)
        };

        scene.add_object(
            mat_uid.as_str(),
            &prims,
            scaled_size / 2.0,
            Vector2f::new(atlas_stride_x, atlas_stride_y),
            z_index,
            if can_occlude_light { 1.0 } else { 0.0 },
            Transform2d::default(),
        )
    }

    fn render_static_object(&mut self, id: &Uuid, scale_factor: f32) {
        let static_obj = self.static_objects.get(id).expect("Static object was missing");
        let mut render_obj = match static_obj.render_obj {
            Some(obj_id) => {
                self.get_scene().get_object(obj_id)
                    .expect("Render object was missing for static object")
            }
            None => {
                let world_id = self.get_world_id().to_string();
                let mut scene = self.get_scene();
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
                    &mut scene,
                    sprite,
                    size,
                    z_index,
                    can_occlude_light,
                    scale_factor,
                );
                static_obj.render_obj = Some(new_obj_handle);

                let mut new_obj = scene.get_object(new_obj_handle)
                    .expect("Render object was missing for static object");
                new_obj.set_transform(render_transform.into());

                new_obj
            }
        };

        // upgrade object reference to mutable
        let static_obj = self.static_objects.get_mut(id).expect("Static object was missing");
        static_obj.get_sprite_mut().update_current_frame(&mut render_obj);
    }

    fn render_actor(&mut self, id: &Uuid, scale_factor: f32) {
        let actor = self.actors.get(id).expect("Actor was missing");

        let render_obj_handle = match actor.render_obj {
            Some(obj_handle) => obj_handle,
            None => {
                let world_id = self.get_world_id().to_string();
                let mut scene = self.get_scene();
                let size = actor.get_size();
                let z_index = actor.get_z_index();
                // upgrade reference
                let actor = self.actors.get_mut(id).expect("Actor was missing");
                let can_occlude_light = actor.can_occlude_light.read().value;
                let sprite = actor.get_sprite_mut();

                let new_obj_handle = Self::create_render_object(
                    world_id,
                    &mut scene,
                    sprite,
                    size,
                    z_index,
                    can_occlude_light,
                    scale_factor,
                );
                actor.render_obj = Some(new_obj_handle);

                new_obj_handle
            }
        };

        let mut render_obj = self.get_scene().get_object(render_obj_handle)
            .expect("Render object for actor was missing");

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
        if camera_transform.dirty {
            self.get_render_camera()
                .set_transform(self
                    .get_render_transform(
                        &camera_transform.value,
                        scale_factor,
                        true,
                    )
                );
        }

        let mut scene = self.get_scene();

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
            self.render_static_object(&obj_id, scale_factor);
        }

        for actor in self.actors.values_mut() {
            if !actor.get_sprite().is_current_animation_static() {
                actor.get_sprite_mut().advance_animation();
            }
        }
        let actors_to_render = self.actors.keys().cloned().collect::<Vec<_>>();
        for actor_id in actors_to_render {
            self.render_actor(&actor_id, scale_factor);
        }
    }
}
