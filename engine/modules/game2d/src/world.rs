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
use crate::world_layer::World2dLayer;
use lazy_static::lazy_static;
use std::collections::HashMap;
use std::ptr;
use std::sync::{Arc, RwLock};
use std::time::Duration;
use argus_scripting_bind::script_bind;
use uuid::Uuid;
use argus_render::common::{RenderCanvas, Transform2d};
use argus_util::dirtiable::Dirtiable;
use argus_util::math::{Vector2f, Vector3f};
use argus_wm::WindowManager;
use crate::light_point::PointLight;
use crate::static_object::StaticObject2d;

const MAX_BACKGROUND_LAYERS: u32 = 16;
const FG_LAYER_ID: &str = "_foreground";
const BG_LAYER_ID_PREFIX: &str = "_background_";

lazy_static! {
    static ref g_worlds: Arc<RwLock<HashMap<String, Arc<RwLock<World2d>>>>> =
        Arc::new(RwLock::new(HashMap::new()));
}

#[script_bind(ref_only)]
pub struct World2d {
    id: String,
    pub(crate) window_id: String,
    scale_factor: f32,
    al_level: Dirtiable<f32>,
    al_color: Dirtiable<Vector3f>,

    fg_layer: World2dLayer,
    bg_layers: [Option<World2dLayer>; MAX_BACKGROUND_LAYERS as usize],
    bg_layers_count: u32,

    abstract_camera: Dirtiable<Transform2d>,
}

#[script_bind]
impl World2d {
    pub fn create(id: String, canvas: &mut RenderCanvas, scale_factor: f32) -> Arc<RwLock<World2d>> {
        let world = Self {
            id: id.clone(),
            scale_factor,
            al_level: Default::default(),
            al_color: Default::default(),
            fg_layer: World2dLayer::new(
                id.clone(),
                FG_LAYER_ID.to_string(),
                1000,
                1.0,
                None,
                true,
                canvas,
            ),
            window_id: canvas.get_window_id().to_string(),
            bg_layers: Default::default(),
            bg_layers_count: 0,
            abstract_camera: Default::default(),
        };

        g_worlds.write().unwrap().insert(id.clone(), Arc::new(RwLock::new(world)));
        g_worlds.read().unwrap().get(&id).unwrap().clone()
    }

    #[script_bind(rename = "create")]
    pub fn create_unsafe<'a>(id: String, window_id: &str, scale_factor: f32) -> &'a mut World2d {
        let mut window = WindowManager::instance().get_window_mut(window_id).unwrap();
        let canvas = window.get_canvas_mut().expect("Window does not have associated canvas");
        let arc = Self::create(
            id,
            canvas.as_any_mut().downcast_mut::<RenderCanvas>()
                .expect("Canvas object from window was unexpected type!"),
            scale_factor,
        );
        let mut guard = arc.write();
        unsafe { &mut *ptr::from_mut(guard.as_deref_mut().unwrap()) }
    }

    pub fn get(id: &str) -> Result<Arc<RwLock<World2d>>, &'static str> {
        match g_worlds.read().expect("Failed to acquire lock for worlds list").get(id) {
            Some(world) => Ok(world.clone()),
            None => Err("Unknown world ID"),
        }
    }

    #[script_bind(rename = "get")]
    pub fn get_unsafe<'a>(id: &str) -> &'a mut World2d {
        let arc = Self::get(id).unwrap();
        let mut guard = arc.write();
        unsafe { &mut *ptr::from_mut(guard.as_deref_mut().unwrap()) }
    }

    pub fn get_or_crash(id: &str) -> Arc<RwLock<World2d>> {
        match Self::get(id) {
            Ok(world) => world,
            Err(e) => panic!("{}", e),
        }
    }

    pub fn get_id(&self) -> &str {
        self.id.as_str()
    }

    pub fn get_scale_factor(&self) -> f32 {
        self.scale_factor
    }

    #[script_bind]
    pub fn get_camera_transform(&self) -> Transform2d {
        let val = self.abstract_camera.peek().value;
        val
    }

    #[script_bind]
    pub fn set_camera_transform(&mut self, transform: Transform2d) {
        self.abstract_camera.set(transform);
    }

    #[script_bind]
    pub fn get_ambient_light_level(&self) -> f32 {
        self.al_level.peek().value
    }

    #[script_bind]
    pub fn set_ambient_light_level(&mut self, level: f32) {
        self.al_level.set(level);
    }

    #[script_bind]
    pub fn get_ambient_light_color(&self) -> Vector3f {
        self.al_color.peek().value
    }

    #[script_bind]
    pub fn set_ambient_light_color(&mut self, color: Vector3f) {
        self.al_color.set(color);
    }

    pub fn get_background_layer(&self, index: u32) -> Result<&World2dLayer, &'static str> {
        if index >= self.bg_layers_count {
            return Err("Invalid background index requested");
        }
        Ok(self.bg_layers[index as usize].as_ref().expect("Background layer is missing"))
    }

    pub fn add_background_layer(
        &mut self,
        parallax_coeff: f32,
        repeat_interval: Option<Vector2f>,
    ) -> Result<&mut World2dLayer, &'static str> {
        if self.bg_layers_count >= MAX_BACKGROUND_LAYERS {
            return Err("Too many background layers added");
        }

        let bg_index = self.bg_layers_count;

        let layer_id = format!("{}{}", BG_LAYER_ID_PREFIX, bg_index);

        let mut window = WindowManager::instance().get_window_mut(&self.window_id).unwrap();
        let canvas = window.get_canvas_mut().expect("Window does not have associated canvas");

        let layer = World2dLayer::new(
            self.id.clone(),
            layer_id,
            100 + bg_index,
            parallax_coeff,
            repeat_interval,
            false,
            canvas.as_any_mut().downcast_mut::<RenderCanvas>()
                .expect("Canvas object from window was unexpected type!"),
        );

        assert!(self.bg_layers[bg_index as usize].is_none());
        self.bg_layers[bg_index as usize] = Some(layer);
        self.bg_layers_count += 1;

        Ok(self.bg_layers[bg_index as usize].as_mut().expect("Background layer is missing"))
    }

    #[script_bind(rename = "add_background_layer")]
    pub fn add_background_layer_or_die(
        &mut self,
        parallax_coeff: f32,
    ) -> &mut World2dLayer {
        self.add_background_layer(parallax_coeff, None).unwrap()
    }

    fn simulate(&mut self, delta: Duration) {
        for i in 0..self.bg_layers_count {
            let layer = self.bg_layers[i as usize].as_mut().expect("Background layer is missing");
            layer.simulate(delta);
        }
        self.fg_layer.simulate(delta);
    }

    fn render(&mut self) {
        let scale_factor = self.get_scale_factor();
        let camera_transform = self.abstract_camera.read();
        let al_level = self.al_level.read();
        let al_color = self.al_color.read();

        for i in 0..self.bg_layers_count {
            self.bg_layers[i as usize].as_mut().expect("Background layer is missing")
                .render(scale_factor, &camera_transform, &al_level, &al_color);
        }

        self.fg_layer.render(scale_factor, &camera_transform, &al_level, &al_color);
    }

    pub(crate) fn simulate_worlds(delta: Duration) {
        for (_, world) in g_worlds.read().unwrap().iter() {
            world.write().unwrap().simulate(delta);
        }
    }

    pub(crate) fn render_worlds(_delta: Duration) {
        for (_, world) in g_worlds.read().unwrap().iter() {
            world.write().unwrap().render();
        }
    }

    fn get_foreground_layer(&self) -> &World2dLayer {
        &self.fg_layer
    }

    fn get_foreground_layer_mut(&mut self) -> &mut World2dLayer {
        &mut self.fg_layer
    }

    pub fn add_collision_layer(&mut self, layer: impl Into<String>) -> Result<(), String> {
        self.fg_layer.add_collision_layer(layer)
    }

    #[script_bind(rename = "add_collision_layer")]
    pub fn add_collision_layer_or_die(&mut self, layer: String) {
        self.fg_layer.add_collision_layer(layer).unwrap()
    }

    pub fn get_static_object(&self, id: &Uuid) -> Result<&StaticObject2d, &'static str> {
        self.get_foreground_layer().get_static_object(id)
    }

    pub fn get_static_object_mut(&mut self, id: &Uuid) -> Result<&mut StaticObject2d, &'static str> {
        self.get_foreground_layer_mut().get_static_object_mut(id)
    }

    pub fn create_static_object(
        &mut self,
        sprite: String,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
        transform: Transform2d,
        collision_layer: impl AsRef<str>,
        collision_mask: &[&str],
    ) -> Result<Uuid, String> {
        self.get_foreground_layer_mut().create_static_object(
            sprite.as_str(),
            size,
            z_index,
            can_occlude_light,
            transform,
            collision_layer,
            collision_mask,
        )
    }

    #[script_bind(rename = "create_static_object")]
    pub fn create_static_object_or_die(
        &mut self,
        sprite: String,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
        transform: Transform2d,
        collision_layer: String,
    ) -> String {
        let coll_mask: &[&str] = &[];
        self.create_static_object(
            sprite,
            size,
            z_index,
            can_occlude_light,
            transform,
            &collision_layer,
            coll_mask,
        ).unwrap()
            .to_string()
    }

    pub fn delete_static_object(&mut self, id: &Uuid) -> Result<(), &'static str> {
        self.get_foreground_layer_mut().delete_static_object(id)
    }

    pub fn get_actor(&self, id: &Uuid) -> Result<&Actor2d, &'static str> {
        self.get_foreground_layer().get_actor(id)
    }

    pub fn get_actor_mut(&mut self, id: &Uuid) -> Result<&mut Actor2d, &'static str> {
        self.get_foreground_layer_mut().get_actor_mut(id)
    }

    #[script_bind(rename = "get_actor_mut")]
    pub fn get_actor_mut_or_die(&mut self, id: &str) -> &mut Actor2d {
        self.get_actor_mut(&Uuid::parse_str(id).unwrap()).unwrap()
    }

    pub fn create_actor(
        &mut self,
        sprite: String,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
        collision_layer: impl AsRef<str>,
        collision_mask: &[impl AsRef<str>],
    ) -> Result<Uuid, String> {
        self.get_foreground_layer_mut().create_actor(
            sprite,
            size,
            z_index,
            can_occlude_light,
            collision_layer,
            collision_mask,
        )
    }

    #[script_bind(rename = "create_actor")]
    pub fn create_actor_or_die(
        &mut self,
        sprite: String,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
        collision_layer: String,
        collision_mask: String,
    ) -> String {
        self.create_actor(
            sprite,
            size,
            z_index,
            can_occlude_light,
            &collision_layer,
            &[&collision_mask],
        ).unwrap()
            .to_string()
    }

    pub fn delete_actor(&mut self, id: &Uuid) -> Result<(), &'static str> {
        self.get_foreground_layer_mut().delete_actor(id)
    }
    
    pub fn add_point_light(&mut self, light: PointLight) -> Result<Uuid, String> {
        self.fg_layer.add_point_light(light)
    }

    #[script_bind(rename = "add_point_light")]
    pub fn add_point_light_or_die(&mut self, light: PointLight) -> String {
        self.fg_layer.add_point_light(light).unwrap().to_string()
    }

    pub fn delete_point_light(&mut self, id: &Uuid) -> Result<(), &'static str> {
        self.fg_layer.delete_point_light(&id)
    }
}
