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
 * MERCHANTABILITY or FITNESS FOR A PARTICULARpub(crate)  PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
use box2d_sys::b2BodyId;
use crate::actor::Actor2d;
use crate::world_layer::World2dLayer;
use lazy_static::lazy_static;
use std::collections::HashMap;
use std::ptr;
use std::sync::{Arc, LazyLock, Mutex, RwLock};
use std::time::Duration;
use box2d_sys::{b2Atan2, b2BodyDef, b2BodyType, b2BodyType_b2_dynamicBody, b2BodyType_b2_staticBody, b2Body_GetPosition, b2Body_GetRotation, b2Body_GetTransform, b2Body_SetLinearVelocity, b2Capsule, b2Circle, b2ComputeCosSin, b2CreateBody, b2CreateCapsuleShape, b2CreateCircleShape, b2CreatePolygonShape, b2CreateWorld, b2DefaultBodyDef, b2DefaultShapeDef, b2DefaultWorldDef, b2MakeBox, b2MakeOffsetBox, b2Rot, b2Vec2, b2Vec2_zero, b2WorldId, b2World_Step};
use fragile::Fragile;
use argus_scripting_bind::script_bind;
use uuid::Uuid;
use argus_render::common::{RenderCanvas, Transform2d};
use argus_util::dirtiable::Dirtiable;
use argus_util::math::{Vector2f, Vector3f};
use argus_wm::WindowManager;
use crate::physics::{BoundingShape};
use crate::light_point::PointLight;
use crate::object::CommonObjectProperties;
use crate::static_object::StaticObject2d;

static g_b2_worlds: LazyLock<Mutex<HashMap<String, Fragile<box2d_sys::b2WorldId>>>> =
    LazyLock::new(|| Mutex::new(HashMap::new()));

const MAX_SECONDARY_LAYERS: u32 = 16;
const PRIM_LAYER_ID: &str = "_primary";
const SEC_LAYER_ID_PREFIX: &str = "_secondary_";

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

    prim_layer: World2dLayer,
    sec_layers: [Option<World2dLayer>; MAX_SECONDARY_LAYERS as usize],
    sec_layers_count: u32,

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
            prim_layer: World2dLayer::new(
                id.clone(),
                PRIM_LAYER_ID.to_string(),
                1000,
                1.0,
                None,
                true,
                canvas,
            ),
            window_id: canvas.get_window_id().to_string(),
            sec_layers: Default::default(),
            sec_layers_count: 0,
            abstract_camera: Default::default(),
        };

        let b2_world = unsafe { b2CreateWorld(&b2DefaultWorldDef()) };
        g_b2_worlds.lock().unwrap()
            .insert(format!("{}|{}", id.clone(), PRIM_LAYER_ID.to_string()), Fragile::new(b2_world));

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

    pub fn get_secondary_layer(&self, index: u32) -> Result<&World2dLayer, &'static str> {
        if index >= self.sec_layers_count {
            return Err("Invalid secondary layer index requested");
        }
        Ok(self.sec_layers[index as usize].as_ref().expect("Secondary layer is missing"))
    }

    pub fn add_secondary_layer(
        &mut self,
        parallax_coeff: f32,
        repeat_interval: Option<Vector2f>,
    ) -> Result<&mut World2dLayer, &'static str> {
        if self.sec_layers_count >= MAX_SECONDARY_LAYERS {
            return Err("Too many secondary layers added");
        }

        let sec_index = self.sec_layers_count;

        let layer_id = format!("{}{}", SEC_LAYER_ID_PREFIX, sec_index);

        let mut window = WindowManager::instance().get_window_mut(&self.window_id).unwrap();
        let canvas = window.get_canvas_mut().expect("Window does not have associated canvas");

        let layer = World2dLayer::new(
            self.id.clone(),
            layer_id,
            100 + sec_index,
            parallax_coeff,
            repeat_interval,
            false,
            canvas.as_any_mut().downcast_mut::<RenderCanvas>()
                .expect("Canvas object from window was unexpected type!"),
        );

        assert!(self.sec_layers[sec_index as usize].is_none());
        self.sec_layers[sec_index as usize] = Some(layer);
        self.sec_layers_count += 1;

        Ok(self.sec_layers[sec_index as usize].as_mut().expect("Secondary layer is missing"))
    }

    #[script_bind(rename = "add_secondary_layer")]
    pub fn add_secondary_layer_or_die(
        &mut self,
        parallax_coeff: f32,
    ) -> &mut World2dLayer {
        self.add_secondary_layer(parallax_coeff, None).unwrap()
    }

    fn simulate(&mut self, delta: Duration) {
        let mut b2_world_map = g_b2_worlds.lock().unwrap();
        let b2_world = b2_world_map.get_mut(&format!("{}|{}", self.id, PRIM_LAYER_ID.to_string()))
            .unwrap().get();
        Self::simulate_layer(&mut self.prim_layer, *b2_world, delta);
    }

    fn simulate_layer(
        layer: &mut World2dLayer,
        b2_world: b2WorldId,
        delta: Duration
    ) {
        for obj in layer.static_objects.values_mut() {
            if obj.common.b2_body.is_none() {
                let transform = &obj.transform;
                let cos_sin = unsafe { b2ComputeCosSin(transform.rotation) };

                let mut body_def = unsafe { b2DefaultBodyDef() };
                body_def.type_ = b2BodyType_b2_staticBody;
                body_def.position = b2Vec2 { x: transform.translation.x, y: transform.translation.y };
                body_def.rotation = b2Rot { c: cos_sin.cosine, s: cos_sin.sine };
                let body_id = unsafe { b2CreateBody(b2_world, &body_def) };

                if let Some(bounding_shape) = obj.bounding_shape {
                    Self::create_shape(&obj.common, &bounding_shape, body_id);
                }

                obj.common.b2_body = Some(body_id);
            }
            let body_handle = obj.common.b2_body.unwrap();
        }

        for actor in layer.actors.values_mut() {
            if actor.common.b2_body.is_none() {
                let transform = actor.transform.peek().value;
                let cos_sin = unsafe { b2ComputeCosSin(transform.rotation) };
                let mut body_def = unsafe { b2DefaultBodyDef() };
                body_def.type_ = b2BodyType_b2_dynamicBody;
                body_def.position = b2Vec2 {
                    x: transform.translation.x,
                    y: transform.translation.y,
                };
                body_def.rotation = b2Rot { c: cos_sin.cosine, s: cos_sin.sine };
                body_def.linearVelocity = b2Vec2 {
                    x: actor.velocity.x,
                    y: actor.velocity.y,
                };
                body_def.angularVelocity = 0.0;
                body_def.fixedRotation = true;
                let body_id = unsafe { b2CreateBody(b2_world, &body_def) };
                if let Some(bounding_shape) = actor.bounding_shape.peek().value {
                    Self::create_shape(&actor.common, &bounding_shape, body_id);
                }
                actor.common.b2_body = Some(body_id);
            }
            let body_handle = actor.common.b2_body.unwrap();

            unsafe {
                b2Body_SetLinearVelocity(
                    body_handle,
                    b2Vec2 { x: actor.velocity.x, y: actor.velocity.y }
                );
            }
        }
        unsafe { b2World_Step(b2_world, delta.as_secs_f32(), 4) };

        for actor in layer.actors.values_mut() {
            let body_transform = unsafe { b2Body_GetTransform(actor.common.b2_body.unwrap()) };
            actor.transform.set(Transform2d::new(
                Vector2f { x: body_transform.p.x, y: body_transform.p.y },
                actor.transform.peek().value.scale,
                unsafe { b2Atan2(body_transform.q.s, body_transform.q.c) },
            ));
        }
    }

    fn create_shape(
        obj_props: &CommonObjectProperties,
        bounding_shape: &BoundingShape,
        body_id: b2BodyId
    ) {
        let mut shape = unsafe { b2DefaultShapeDef() };
        shape.density = 1.0;
        shape.friction = 0.0;
        shape.filter.categoryBits = obj_props.collision_layer;
        shape.filter.maskBits = obj_props.collision_mask;

        match bounding_shape {
            BoundingShape::Rectangle(rect) => {
                let polygon = unsafe { b2MakeBox(rect.size.x / 2.0, rect.size.y / 2.0) };
                unsafe { b2CreatePolygonShape(body_id, &shape, &polygon) };
            }
            BoundingShape::Capsule(cap) => {
                let radius = cap.width / 2.0;
                let center1 = b2Vec2 { x: cap.offset.x, y: -cap.length / 2.0 + radius + cap.offset.y };
                let center2 = b2Vec2 { x: cap.offset.x, y: cap.length / 2.0 - radius + cap.offset.y };
                let capsule = unsafe { b2Capsule { center1, center2, radius } };
                unsafe { b2CreateCapsuleShape(body_id, &shape, &capsule) };
            }
            BoundingShape::Circle(cir) => {
                let circle = b2Circle {
                    center: b2Vec2 { x: 0.0, y: 0.0 },
                    radius: cir.radius
                };
                unsafe { b2CreateCircleShape(body_id, &shape, &circle) };
            }
        }
    }

    fn render(&mut self) {
        let scale_factor = self.get_scale_factor();
        let camera_transform = self.abstract_camera.read();
        let al_level = self.al_level.read();
        let al_color = self.al_color.read();

        for i in 0..self.sec_layers_count {
            self.sec_layers[i as usize].as_mut().expect("Secondary layer is missing")
                .render(scale_factor, &camera_transform, &al_level, &al_color);
        }

        self.prim_layer.render(scale_factor, &camera_transform, &al_level, &al_color);
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

    fn get_primary_layer(&self) -> &World2dLayer {
        &self.prim_layer
    }

    fn get_primary_layer_mut(&mut self) -> &mut World2dLayer {
        &mut self.prim_layer
    }

    pub fn add_collision_layer(&mut self, layer: impl Into<String>) -> Result<(), String> {
        self.prim_layer.add_collision_layer(layer)
    }

    #[script_bind(rename = "add_collision_layer")]
    pub fn add_collision_layer_or_die(&mut self, layer: String) {
        self.prim_layer.add_collision_layer(layer).unwrap()
    }

    pub fn get_static_object(&self, id: &Uuid) -> Result<&StaticObject2d, &'static str> {
        self.get_primary_layer().get_static_object(id)
    }

    pub fn get_static_object_mut(&mut self, id: &Uuid) -> Result<&mut StaticObject2d, &'static str> {
        self.get_primary_layer_mut().get_static_object_mut(id)
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
        self.get_primary_layer_mut().create_static_object(
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
        collision_layer: &str,
        collision_mask: &str,
    ) -> String {
        self.create_static_object(
            sprite,
            size,
            z_index,
            can_occlude_light,
            transform,
            collision_layer,
            &[collision_mask],
        ).unwrap()
            .to_string()
    }

    pub fn delete_static_object(&mut self, id: &Uuid) -> Result<(), &'static str> {
        self.get_primary_layer_mut().delete_static_object(id)
    }

    pub fn get_actor(&self, id: &Uuid) -> Result<&Actor2d, &'static str> {
        self.get_primary_layer().get_actor(id)
    }

    pub fn get_actor_mut(&mut self, id: &Uuid) -> Result<&mut Actor2d, &'static str> {
        self.get_primary_layer_mut().get_actor_mut(id)
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
        self.get_primary_layer_mut().create_actor(
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
        collision_layer: &str,
        collision_mask: &str,
    ) -> String {
        self.create_actor(
            sprite,
            size,
            z_index,
            can_occlude_light,
            collision_layer,
            &[collision_mask],
        ).unwrap()
            .to_string()
    }

    pub fn delete_actor(&mut self, id: &Uuid) -> Result<(), &'static str> {
        self.get_primary_layer_mut().delete_actor(id)
    }
    
    pub fn add_point_light(&mut self, light: PointLight) -> Result<Uuid, String> {
        self.prim_layer.add_point_light(light)
    }

    #[script_bind(rename = "add_point_light")]
    pub fn add_point_light_or_die(&mut self, light: PointLight) -> String {
        self.prim_layer.add_point_light(light).unwrap().to_string()
    }

    pub fn delete_point_light(&mut self, id: &Uuid) -> Result<(), &'static str> {
        self.prim_layer.delete_point_light(&id)
    }
}
