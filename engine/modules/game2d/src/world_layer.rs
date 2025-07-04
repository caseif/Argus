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
use crate::light_point::PointLight;
use crate::sprite::Sprite;
use crate::static_object::StaticObject2d;
use argus_render::common::{Material, RenderCanvas, TextureData, Transform2d, Vertex2d};
use argus_render::constants::RESOURCE_TYPE_MATERIAL;
use argus_render::twod::{get_render_context_2d, RenderPrimitive2d, Scene2d};
use argus_resman::ResourceManager;
use argus_scripting_bind::script_bind;
use argus_util::dirtiable::ValueAndDirtyFlag;
use argus_util::math::{Vector2f, Vector3f};
use argus_util::pool::Handle;
use std::collections::{hash_map, HashMap};
use std::ops::DerefMut;
use uuid::Uuid;

const LAYER_PREFIX: &str = "_worldlayer_";

//const COLLISION_ARROW_TTL: u32 = 20;

//const LAST_COLLISIONS: LazyLock<Mutex<HashMap<(Uuid, Uuid), CollisionResolution>>> =
//    LazyLock::new(|| Mutex::new(HashMap::new()));

#[script_bind(ref_only)]
pub struct World2dLayer {
    world_id: String,

    z_index: u32,
    parallax_coeff: f32,
    repeat_interval: Option<Vector2f>,

    scene_id: String,
    render_camera_id: String,

    pub(crate) static_objects: HashMap<Uuid, StaticObject2d>,
    pub(crate) actors: HashMap<Uuid, Actor2d>,
    point_lights: HashMap<Uuid, PointLight>,

    collision_layers: HashMap<String, u64>,
    next_collision_layer_bit: u64,
}

#[script_bind]
impl World2dLayer {
    pub(crate) fn new(
        world_id: String,
        z_index: u32,
        parallax_coeff: f32,
        repeat_interval: Option<Vector2f>,
        lighting_enabled: bool,
        canvas: &mut RenderCanvas,
    ) -> World2dLayer {
        let layer_uuid = Uuid::new_v4().to_string();
        let layer_id_str = format!("{}{}_{}", LAYER_PREFIX, world_id, layer_uuid);

        let mut scene = get_render_context_2d()
            .create_scene(layer_id_str.as_str());
        scene.set_lighting_enabled(lighting_enabled);

        let scene_id = scene.get_id().to_string();
        let render_camera = scene.create_camera(layer_id_str.as_str());
        canvas.add_default_viewport_2d(
            scene_id,
            render_camera.get_id(),
            z_index
        ).expect("Failed to create viewport");

        let layer = World2dLayer {
            world_id,
            z_index,
            parallax_coeff,
            repeat_interval,
            scene_id: layer_id_str.clone(),
            render_camera_id: layer_id_str,
            static_objects: Default::default(),
            actors: Default::default(),
            point_lights: Default::default(),
            collision_layers: Default::default(),
            next_collision_layer_bit: 1,
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

    pub fn add_collision_layer(&mut self, layer_id: impl Into<String>) -> Result<(), String> {
        if self.next_collision_layer_bit == 0 {
            return Err("World layer may not have more than 64 collision layers".to_owned());
        }

        let hash_map::Entry::Vacant(entry) = self.collision_layers.entry(layer_id.into()) else {
            return Err("Collision layer with same ID already exists in world layer".to_owned());
        };

        entry.insert(self.next_collision_layer_bit);

        if self.next_collision_layer_bit == (1 << 63) {
            self.next_collision_layer_bit = 0;
        } else {
            self.next_collision_layer_bit <<= 1;
        }

        Ok(())
    }

    #[script_bind(rename = "add_collision_layer")]
    pub fn add_collision_layer_or_die(&mut self, layer_id: String) {
        self.add_collision_layer(layer_id).unwrap();
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
        collision_layer: impl AsRef<str>,
        collision_mask: &[&str],
    ) -> Result<Uuid, String> {
        let sprite_defn_res = match ResourceManager::instance().get_resource(sprite) {
            Ok(res) => res,
            Err(e) => { return Err(e.info); }
        };

        println!("collision_layers: {:?}", self.collision_layers);
        println!("collision_layer: {:?}", collision_layer.as_ref());
        let collision_bit = match self.collision_layers.get(collision_layer.as_ref()) {
            Some(bit) => *bit,
            None => {
                return Err("Collision layer for static object does not exist".to_owned());
            },
        };
        let collision_bitmask = self.get_collision_bitmask(collision_mask)?;

        let obj = StaticObject2d::new(
            sprite_defn_res,
            size,
            z_index,
            can_occlude_light,
            transform,
            collision_bit,
            collision_bitmask,
        );
        let id = Uuid::new_v4();
        self.static_objects.insert(id, obj);
        Ok(id)
    }

    #[script_bind(rename = "create_static_object")]
    pub fn create_static_object_or_die(
        &mut self,
        sprite: &str,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
        transform: Transform2d,
        collision_layer: &str,
    ) -> String {
        self.create_static_object(
            sprite,
            size,
            z_index,
            can_occlude_light,
            transform,
            collision_layer,
            &[],
        )
            .unwrap()
            .to_string()
    }

    pub fn delete_static_object(&mut self, id: &Uuid) -> Result<(), &'static str> {
        match self.static_objects.remove(&id) {
            Some(obj) => {
                if let Some(render_obj) = obj.common.render_obj {
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
        collision_layer: impl AsRef<str>,
        collision_mask: &[impl AsRef<str>],
    ) -> Result<Uuid, String> {
        let sprite_defn_res = match ResourceManager::instance().get_resource(&sprite_uid) {
            Ok(res) => res,
            Err(e) => { return Err(e.info); }
        };

        let collision_bit = match self.collision_layers.get(collision_layer.as_ref()) {
            Some(bit) => *bit,
            None => {
                return Err("Collision layer for actor does not exist".to_owned());
            },
        };
        let collision_bitmask = self.get_collision_bitmask(collision_mask)?;

        let actor = Actor2d::new(
            sprite_defn_res,
            size,
            z_index,
            can_occlude_light,
            collision_bit,
            collision_bitmask,
        );
        let id = Uuid::new_v4();
        self.actors.insert(id, actor);
        Ok(id)
    }

    pub fn delete_actor(&mut self, id: &Uuid) -> Result<(), &'static str> {
        match self.actors.remove(id) {
            Some(actor) => {
                if let Some(render_obj) = actor.common.render_obj {
                    let context = get_render_context_2d();
                    context.get_scene_mut(self.get_scene_id()).unwrap().remove_object(render_obj);
                }

                Ok(())
            }
            None => Err("No such actor exists for world layer (in delete_actor)"),
        }
    }

    pub fn get_point_light(&self, id: &Uuid) -> Result<&PointLight, &'static str> {
        match self.point_lights.get(id) {
            Some(light) => Ok(light),
            None => Err("No such point light exists for world layer (in get_point_light)"),
        }
    }

    pub fn get_point_light_mut(&mut self, id: &Uuid) -> Result<&mut PointLight, &'static str> {
        match self.point_lights.get_mut(id) {
            Some(light) => Ok(light),
            None => Err("No such point light exists for world layer (in get_point_light_mut)")
        }
    }

    pub fn add_point_light(&mut self, light: PointLight) -> Result<Uuid, String> {
        let uuid = Uuid::new_v4();
        self.point_lights.insert(uuid, light);

        Ok(uuid)
    }

    pub fn delete_point_light(&mut self, id: &Uuid) -> Result<(), &'static str> {
        match self.point_lights.remove(id) {
            Some(light) => {
                if let Some(render_light) = light.render_light {
                    let context = get_render_context_2d();
                    context.get_scene_mut(self.get_scene_id()).unwrap().remove_light(render_light);
                }

                Ok(())
            }
            None => Err("No such point light exists for world layer (in delete_point_light)"),
        }
    }

    fn get_render_transform(
        world_transform: &Transform2d,
        _world_size: &Vector2f,
        world_scale_factor: f32,
        parallax_coeff: f32,
    ) -> Transform2d {
        Transform2d::new(
            (world_transform.translation * parallax_coeff) / world_scale_factor,
            world_transform.scale,
            world_transform.rotation,
        )
    }

    fn create_render_object(
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
        let half_size = scaled_size / 2.0;

        let v1 = Vertex2d {
            position: Vector2f::new(-half_size.x, -half_size.y),
            tex_coord: Vector2f::new(0.0, 0.0),
            normal: Default::default(),
            color: Default::default(),
        };
        let v2 = Vertex2d {
            position: Vector2f::new(-half_size.x, half_size.y),
            tex_coord: Vector2f::new(0.0, 1.0),
            normal: Default::default(),
            color: Default::default(),
        };
        let v3 = Vertex2d {
            position: Vector2f::new(half_size.x, half_size.y),
            tex_coord: Vector2f::new(1.0, 1.0),
            normal: Default::default(),
            color: Default::default(),
        };
        let v4 = Vertex2d {
            position: Vector2f::new(half_size.x, -half_size.y),
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
            Vector2f::new(0.0, 0.0),
            Vector2f::new(atlas_stride_x, atlas_stride_y),
            z_index,
            if can_occlude_light { 1.0 } else { 0.0 },
            Transform2d::default(),
        )
    }

    fn render_static_object(&mut self, scene: &mut Scene2d, id: &Uuid, scale_factor: f32) {
        let static_obj = self.static_objects.get(id).expect("Static object was missing");
        let mut render_obj = match static_obj.common.render_obj {
            Some(obj_id) => {
                scene.get_object_mut(obj_id).expect("Render object was missing for static object")
            }
            None => {
                let size = static_obj.get_size();
                let z_index = static_obj.get_z_index();
                let can_occlude_light = static_obj.get_can_occlude_light();
                let render_transform = Self::get_render_transform(
                    &static_obj.get_transform(),
                    &static_obj.common.size,
                    scale_factor,
                    1.0,
                );
                // upgrade reference
                let static_obj = self.static_objects.get_mut(id)
                    .expect("Static object was missing");
                let sprite = &mut static_obj.common.sprite;

                let new_obj_handle = Self::create_render_object(
                    scene,
                    sprite,
                    size,
                    z_index,
                    can_occlude_light,
                    scale_factor,
                );
                static_obj.common.render_obj = Some(new_obj_handle);

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

        let mut render_obj = match actor.common.render_obj {
            Some(obj_handle) => scene.get_object_mut(obj_handle).unwrap(),
            None => {
                let size = actor.get_size();
                let z_index = actor.get_z_index();
                let render_transform = Self::get_render_transform(
                    &actor.get_transform(),
                    &size,
                    scale_factor,
                    1.0,
                );
                // upgrade reference
                let actor = self.actors.get_mut(id).expect("Actor was missing");
                let can_occlude_light = actor.can_occlude_light.read().value;
                let sprite = &mut actor.common.sprite;

                let new_obj_handle = Self::create_render_object(
                    scene,
                    sprite,
                    size,
                    z_index,
                    can_occlude_light,
                    scale_factor,
                );
                actor.common.render_obj = Some(new_obj_handle);

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

        let size = Vector2f::default(); //actor.get_size();
        if read_transform.dirty {
            render_obj.set_transform(Self::get_render_transform(
                &read_transform.value,
                &size,
                scale_factor,
                1.0,
            ).into());
        }

        // upgrade reference
        let actor = self.actors.get_mut(id).expect("Actor was missing");
        actor.get_sprite_mut().update_current_frame(&mut render_obj);
    }

    pub(crate) fn render_point_light(&mut self, scene: &mut Scene2d, id: &Uuid, scale_factor: f32) {
        let light = self.point_lights.get(id).expect("Point light was missing");

        let mut render_light = match light.render_light {
            Some(light_handle) => scene.get_light_mut(light_handle).unwrap(),
            None => {
                let render_transform = Self::get_render_transform(
                    &light.transform.peek().value,
                    &Vector2f::default(),
                    scale_factor,
                    1.0,
                );
                // upgrade reference
                let light = self.point_lights.get_mut(id).expect("Point light was missing");

                let new_handle = scene.add_light(
                    light.properties.peek().value,
                    render_transform.clone(),
                );
                light.render_light = Some(new_handle);

                let mut new_render_light = scene.get_light_mut(new_handle)
                    .expect("Render light was missing for point light");
                new_render_light.set_transform(render_transform.into());

                new_render_light
            }
        };


        let read_props;
        let read_transform;
        {
            // upgrade reference so we can reset the dirty flags
            let light = self.point_lights.get_mut(id).expect("Actor was missing");
            read_props = light.properties.read();
            read_transform = light.transform.read();
        }

        if read_props.dirty {
            render_light.set_properties(read_props.value);
        }

        if read_transform.dirty {
            render_light.set_transform(
                Self::get_render_transform(
                    &read_transform.value,
                    &Vector2f::default(),
                    scale_factor,
                    1.0,
                ).into()
            );
        }
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
                    Self::get_render_transform(
                        &camera_transform.value,
                        &Vector2f::default(),
                        scale_factor,
                        self.parallax_coeff,
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
        
        let point_lights_to_render = self.point_lights.keys().cloned().collect::<Vec<_>>();
        for light_id in point_lights_to_render {
            self.render_point_light(scene.deref_mut(), &light_id, scale_factor);
        }
    }

    fn get_collision_bitmask(&self, layers: &[impl AsRef<str>]) -> Result<u64, String> {
        let mut collision_bitmask = 0;
        for layer in layers {
            let layer = layer.as_ref();
            match self.collision_layers.get(layer) {
                Some(bit) => {
                    collision_bitmask |= bit;
                },
                None => {
                    return Err("Collision layer in static object mask does not exist".to_owned());
                },
            }
        }
        Ok(collision_bitmask)
    }
}
