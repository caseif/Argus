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
use resman_rustabi::argus::resman::{ResourceError, ResourceErrorReason, ResourceLoader, ResourceManager, ResourcePrototype, WrappedResourceLoader};
use std::ops::Deref;
use lowlevel_rustabi::argus::lowlevel::{Padding, Vector2u};
use serde::{Deserialize, Serialize};
use serde_json::error::Category;
use serde_valid::Validate;
use crate::sprite::{SpriteAnimation, SpriteAnimationFrame, SpriteDefinition};

const MAGIC_ANIM_STATIC: &str = "_static";

pub(crate) struct SpriteLoader;

impl ResourceLoader for SpriteLoader {
    fn load_resource(
        &mut self,
        _handle: WrappedResourceLoader,
        _manager: ResourceManager,
        prototype: ResourcePrototype,
        read_callback: Box<dyn Fn(&mut [u8], usize) -> usize>,
        _size: usize,
    ) -> Result<*mut u8, ResourceError> {
        const BUF_LEN: usize = 1024;
        let mut buf = [0u8; BUF_LEN];
        let mut data: Vec<u8> = Vec::with_capacity(BUF_LEN);
        loop {
            let read_bytes = read_callback.deref()(buf.as_mut_slice(), BUF_LEN);
            if read_bytes == 0 {
                break;
            }

            data.append(&mut buf[0..read_bytes].to_vec());
        }

        let sprite_json = match String::from_utf8(data) {
            Ok(json) => json,
            Err(_) => {
                return Err(ResourceError::new(
                    ResourceErrorReason::MalformedContent,
                    prototype.uid.as_str(),
                    "Sprite definition is not valid UTF-8"
                ))
            }
        };

        match parse_sprite_defn(sprite_json) {
            Ok(defn) => Ok(Box::into_raw(Box::new(defn)).cast()),
            Err(e) => {
                let reason = match e.classify() {
                    Category::Io => ResourceErrorReason::LoadFailed,
                    Category::Syntax => ResourceErrorReason::MalformedContent,
                    Category::Data => ResourceErrorReason::InvalidContent,
                    Category::Eof => ResourceErrorReason::MalformedContent,
                };
                Err(ResourceError::new(
                    reason,
                    prototype.uid.as_str(),
                    format!("Sprite definition '{}' is not valid: {:?}", prototype.uid, e).as_str(),
                ))
            },
        }
    }

    fn copy_resource(
        &mut self,
        _handle: WrappedResourceLoader,
        _manager: ResourceManager,
        _prototype: ResourcePrototype,
        src_data: *const u8,
    ) -> Result<*mut u8, ResourceError> {
        let defn: &SpriteDefinition = unsafe { *src_data.cast() };
        Ok(Box::into_raw(Box::new(defn.clone())).cast())
    }

    fn unload_resource(&mut self, _handle: WrappedResourceLoader, ptr: *mut u8) {
        unsafe {
            _ = Box::from_raw(ptr.cast::<SpriteDefinition>());
        }
    }
}

fn parse_sprite_defn(json_str: String) -> Result<SpriteDefinition, serde_json::Error> {
    let mut raw_defn = serde_json::from_str::<SpriteResourceModel>(json_str.as_str())?;
    if raw_defn.animations.is_empty() {
        let static_anim = SpriteResourceAnimationModel {
            frame_duration: 1.0,
            does_loop: false,
            frames: vec![SpriteResourceAnimationFrameModel {
                x: 0,
                y: 0,
                duration: 1.0,
            }],
            padding: Default::default(),
        };
        raw_defn.animations.insert(MAGIC_ANIM_STATIC.to_string(), static_anim);
        raw_defn.default_animation = MAGIC_ANIM_STATIC.to_string();
    }
    Ok(raw_defn.into())
}

#[derive(Clone, Deserialize, Serialize, Validate)]
struct SpriteResourceModel {
    #[serde(default)]
    #[validate(exclusive_minimum = 0.0)]
    width: f32,
    #[serde(default)]
    #[validate(exclusive_minimum = 0.0)]
    height: f32,
    #[serde(default)]
    default_animation: String,
    #[serde(default)]
    #[validate(exclusive_minimum = 0.0)]
    anim_speed: f32,
    #[serde(default)]
    atlas: String,
    #[serde(default)]
    #[validate(minimum = 1)]
    tile_width: u32,
    #[validate(minimum = 1)]
    #[serde(default)]
    tile_height: u32,
    #[serde(default)]
    animations: HashMap<String, SpriteResourceAnimationModel>
}

#[derive(Clone, Deserialize, Serialize, Validate)]
struct SpriteResourceAnimationModel {
    #[serde(rename(deserialize = "loop"))]
    #[serde(default)]
    does_loop: bool,
    #[serde(default)]
    #[validate(exclusive_minimum = 0.0)]
    frame_duration: f32,
    #[serde(default)]
    padding: SpriteResourceAnimationPaddingModel,
    frames: Vec<SpriteResourceAnimationFrameModel>,
}

#[derive(Clone, Copy, Default, Deserialize, Serialize)]
struct SpriteResourceAnimationPaddingModel {
    #[serde(default)]
    top: u32,
    #[serde(default)]
    bottom: u32,
    #[serde(default)]
    left: u32,
    #[serde(default)]
    right: u32,
}

#[derive(Clone, Copy, Deserialize, Serialize, Validate)]
struct SpriteResourceAnimationFrameModel {
    x: u32,
    y: u32,
    #[serde(default)]
    #[validate(exclusive_minimum = 0.0)]
    duration: f32,
}

impl Into<SpriteDefinition> for SpriteResourceModel {
    fn into(self) -> SpriteDefinition {
        SpriteDefinition {
            def_anim: self.default_animation,
            def_speed: self.anim_speed,
            atlas: self.atlas,
            tile_size: Vector2u::new(self.tile_width, self.tile_height),
            animations: self.animations.into_iter()
                .map(|(id, anim)| (id.clone(), Into::<SpriteAnimation>::into((id, anim))))
                .collect(),
        }
    }
}

impl Into<SpriteAnimation> for (String, SpriteResourceAnimationModel) {
    fn into(self) -> SpriteAnimation {
        let (id, anim) = self;
        let mut real_anim = SpriteAnimation {
            id,
            does_loop: anim.does_loop,
            padding: anim.padding.into(),
            def_duration: anim.frame_duration.into(),
            frames: anim.frames.into_iter().map(|frame| frame.into()).collect(),
        };

        for frame in &mut real_anim.frames {
            if frame.duration <= 0.0 {
                frame.duration = real_anim.def_duration;
            }
        }

        real_anim
    }
}

impl Into<Padding> for SpriteResourceAnimationPaddingModel {
    fn into(self) -> Padding {
        Padding {
            top: self.top,
            bottom: self.bottom,
            left: self.left,
            right: self.right,
        }
    }
}

impl Into<SpriteAnimationFrame> for SpriteResourceAnimationFrameModel {
    fn into(self) -> SpriteAnimationFrame {
        SpriteAnimationFrame {
            offset: Vector2u::new(self.x, self.y),
            duration: self.duration,
        }
    }
}
