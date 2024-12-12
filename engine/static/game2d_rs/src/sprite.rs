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

use std::collections::HashMap;
use std::time::{Duration, Instant};
use argus_scripting_bind::script_bind;
use lowlevel_rustabi::argus::lowlevel::{Dirtiable, Padding, Vector2u};
use render_rustabi::argus::render::RenderObject2d;
use resman_rustabi::argus::resman::Resource;

#[derive(Clone, Debug)]
pub(crate) struct SpriteAnimationFrame {
    pub(crate) offset: Vector2u,
    pub(crate) duration: f32,
}

#[script_bind]
#[derive(Clone, Debug)]
pub(crate) struct SpriteAnimation {
    pub(crate) id: String,

    pub(crate) does_loop: bool,
    pub(crate) padding: Padding,
    pub(crate) def_duration: f32,

    pub(crate) frames: Vec<SpriteAnimationFrame>,
}

impl SpriteAnimation {
    pub(crate) fn get_id(&self) -> &str {
        self.id.as_str()
    }
}

#[derive(Clone, Debug)]
pub(crate) struct SpriteDefinition {
    pub(crate) def_anim: String,
    pub(crate) def_speed: f32,
    pub(crate) atlas: String,
    pub(crate) tile_size: Vector2u,

    pub(crate) animations: HashMap<String, SpriteAnimation>,
}

#[derive(Debug)]
#[script_bind(ref_only)]
pub struct Sprite {
    definition: SpriteDefinition,

    anim_start_offsets: HashMap<String, usize>,

    speed: f32,
    cur_anim_id: String,

    pub(crate) cur_frame: Dirtiable<usize>,
    pub(crate) next_frame_update: Instant,
    pub(crate) paused: bool,
    pub(crate) pending_reset: bool,
}

#[script_bind]
impl Sprite {
    pub fn new(defn_res: Resource) -> Self {
        let defn = defn_res.get::<SpriteDefinition>();
        let mut sprite = Self {
            definition: defn.clone(),
            anim_start_offsets: HashMap::new(),
            speed: defn.def_speed,
            cur_anim_id: defn.def_anim.clone(),
            cur_frame: Dirtiable::new(0),
            next_frame_update: Instant::now(),
            paused: false,
            pending_reset: false,
        };
        sprite.set_current_animation(sprite.definition.def_anim.clone())
            .expect("Initial animation for sprite was missing");
        sprite
    }

    pub(crate) fn get_definition(&self) -> &SpriteDefinition {
        &self.definition
    }

    #[allow(dead_code)]
    pub(crate) fn get_animation_start_offsets(&self) -> &HashMap<String, usize> {
        &self.anim_start_offsets
    }

    pub(crate) fn get_animation_start_offsets_mut(&mut self) -> &mut HashMap<String, usize> {
        &mut self.anim_start_offsets
    }

    #[script_bind]
    pub fn get_animation_speed(&self) -> f32 {
        self.speed
    }

    #[script_bind]
    pub fn set_animation_speed(&mut self, speed: f32) {
        self.speed = speed;
    }

    pub fn get_available_animations(&self) -> Vec<String> {
        self.definition.animations.keys().cloned().collect()
    }

    #[script_bind]
    pub fn get_current_animation_id(&self) -> &str {
        self.cur_anim_id.as_str()
    }

    pub(crate) fn get_current_animation(&self) -> &SpriteAnimation {
        self.definition.animations.get(&self.cur_anim_id).expect("Sprite animation is missing")
    }

    pub(crate) fn set_current_animation(&mut self, animation_id: String)
        -> Result<(), &'static str> {
        match self.definition.animations.get(&animation_id) {
            Some(_) => {
                self.cur_anim_id = animation_id;
                self.cur_frame =
                    Dirtiable::new(self.anim_start_offsets.get(&self.cur_anim_id).cloned()
                        .unwrap_or_default());
                Ok(())
            }
            None => Err("Animation not found by ID")
        }
    }

    #[script_bind(rename = "set_current_animation")]
    pub(crate) fn set_current_animation_or_die(&mut self, animation_id: String) {
        self.set_current_animation(animation_id).expect("Failed to set sprite animation");
    }

    #[script_bind]
    pub fn does_current_animation_loop(&self) -> bool {
        self.get_current_animation().does_loop
    }

    #[script_bind]
    pub fn is_current_animation_static(&self) -> bool {
        self.get_current_animation().frames.len() == 1
    }

    pub fn get_current_animation_padding(&self) -> Padding {
        self.get_current_animation().padding
    }

    #[script_bind]
    pub fn pause_animation(&mut self) {
        self.paused = false;
    }

    #[script_bind]
    pub fn resume_animation(&mut self) {
        self.paused = true;
    }

    #[script_bind]
    pub fn reset_animation(&mut self) {
        self.pending_reset = true;
    }

    pub(crate) fn update_current_frame(&mut self, render_obj: &mut RenderObject2d) {
        let cur_frame = self.cur_frame.read();
        if cur_frame.dirty {
            render_obj.set_active_frame(self.get_current_animation().frames[cur_frame.value].offset);
        }
    }

    pub(crate) fn advance_animation(&mut self) {
        if self.pending_reset {
            self.cur_frame.set(0);
            self.next_frame_update = Instant::now() + self.get_current_frame_duration();
            return;
        }

        if self.paused {
            //TODO: we don't update next_frame_update so the next frame would
            //      likely be displayed immediately after unpausing - we should
            //      store the remaining time instead
            return;
        }

        let anim = self.get_current_animation();
        let last_frame_index = anim.frames.len() - 1;
        // you could theoretically force an infinite loop if now() was evaluated every iteration
        let cur_time = Instant::now();
        while cur_time >= self.next_frame_update {
            if self.cur_frame.peek().value == last_frame_index {
                self.cur_frame.set(0);
            } else {
                self.cur_frame.update(|frame| frame + 1);
            }

            let frame_dur = self.get_current_frame_duration();
            self.next_frame_update += frame_dur;
        }
    }

    fn get_current_frame_duration(&self) -> Duration {
        let cur_frame = &self.get_current_animation().frames[self.cur_frame.peek().value];
        let frame_dur = cur_frame.duration * self.get_animation_speed();
        let dur_s = frame_dur as u64;
        let dur_ns = (frame_dur % 1.0) * 1_000_000_000.0;
        Duration::new(dur_s, dur_ns as u32)
    }
}
