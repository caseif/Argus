/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as_ptr published by
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
 */use fragile::Fragile;
use crate::Window;

use sdl3::video::VkInstance as SdlVkInstance;
use sdl3::video::VkSurfaceKHR as SdlVkSurfaceKHR;

pub struct VkInstance {
    underlying: Fragile<SdlVkInstance>,
}

impl VkInstance {
    pub fn from_raw(handle: u64) -> Self {
        Self {
            underlying: Fragile::new(handle as SdlVkInstance),
        }
    }
    
    pub fn as_raw(&self) -> u64 {
        self.underlying.get().addr() as u64
    }
}

pub struct VkSurfaceKHR {
    underlying: SdlVkSurfaceKHR,
}

impl VkSurfaceKHR {
    pub fn as_raw(&self) -> u64 {
        self.underlying.addr() as u64
    }
}

pub fn vk_is_supported() -> bool {
    true //TODO
}

pub fn vk_create_surface(
    window: &Window,
    instance: &VkInstance,
) -> Result<VkSurfaceKHR, String> {
    let sdl_window = window.handle.as_ref()
        .ok_or_else(|| "SDL window is not yet created".to_owned())?
        .try_get()
        .map_err(|_| "Vulkan surface cannot be created outside of render thread")?;
    let vk_surface = sdl_window
        .vulkan_create_surface(*instance.underlying.get())
        .map_err(|err| format!("Failed to create Vulkan surface: {:?}", err))?;
    Ok(VkSurfaceKHR { underlying: vk_surface })
}

pub fn vk_get_required_instance_extensions(window: &Window) -> Result<Vec<String>, String> {
    let sdl_window = window.handle.as_ref()
        .ok_or_else(|| "SDL window is not yet created".to_owned())?
        .try_get()
        .map_err(|_| "Vulkan surface cannot be created outside of render thread")?;
    sdl_window.vulkan_instance_extensions()
        .map_err(|err| format!("Failed to create Vulkan surface: {:?}", err))
}
