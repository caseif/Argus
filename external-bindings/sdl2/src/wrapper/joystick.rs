use std::{ffi, mem};
use std::ffi::CString;
use num_enum::{IntoPrimitive, TryFromPrimitive};
use crate::bindings::*;
use crate::error::{c_str_to_string_or_error, sdl_get_error, SdlError};

pub struct SdlJoystick {
    pub(crate) instance_id: i32,
    pub(crate) device_index: Option<i32>,
    pub(crate) handle: Option<*mut SDL_Joystick>,
}

impl SdlJoystick {
    fn from_device_index(device_index: i32) -> Result<Self, SdlError> {
        let instance_id = unsafe { SDL_JoystickGetDeviceInstanceID(device_index) };
        if instance_id < 0 {
            return Err(sdl_get_error());
        }

        Ok(Self {
            instance_id,
            device_index: None,
            handle: None,
        })
    }

    pub fn get_all_joysticks() -> Result<Iter, SdlError> {
        let count = unsafe { SDL_NumJoysticks() };
        if count < 0 {
            return Err(sdl_get_error());
        }
        Ok(Iter::new(count as usize))
    }

    pub fn get_name(&self) -> Result<String, SdlError> {
        if let Some(handle) = self.handle {
            c_str_to_string_or_error(
                unsafe { SDL_JoystickName(handle) }
            )
        } else {
            c_str_to_string_or_error(
                unsafe { SDL_JoystickNameForIndex(self.device_index.unwrap() as ffi::c_int) }
            )
        }
    }

    pub fn get_instance_id(&self) -> i32 {
        self.instance_id
    }
    
    pub fn is_game_controller(&self) -> bool {
        if let Some(handle) = self.handle {
            unsafe { SDL_JoystickGetType(handle) == SDL_JOYSTICK_TYPE_GAMECONTROLLER }
        } else {
            unsafe {
                SDL_JoystickGetDeviceType(self.device_index.unwrap()) ==
                    SDL_JOYSTICK_TYPE_GAMECONTROLLER
            }
        }
    }
}

pub struct Iter {
    count: usize,
    cur: usize,
}

impl Iter {
    fn new(count: usize) -> Self {
        unsafe { SDL_LockJoysticks(); }
        Self {
            count,
            cur: 0,
        }
    }
}

impl Iterator for Iter {
    type Item = SdlJoystick;

    fn next(&mut self) -> Option<Self::Item> {
        if self.cur < self.count {
            let joystick = SdlJoystick::from_device_index(self.cur as i32).unwrap();
            self.cur += 1;
            Some(joystick)
        } else {
            None
        }
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        (self.count, Some(self.count))
    }
}

impl Drop for Iter {
    fn drop(&mut self) {
        unsafe { SDL_UnlockJoysticks(); }
    }
}

#[allow(clippy::unnecessary_cast)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive)]
#[repr(i32)]
pub enum SdlJoystickPowerLevel {
    Unknown = SDL_JOYSTICK_POWER_UNKNOWN as i32,
    /// <= 5%
    Empty = SDL_JOYSTICK_POWER_EMPTY as i32,
    /// <= 20%
    Low = SDL_JOYSTICK_POWER_LOW as i32,
    /// <= 70%
    Medium = SDL_JOYSTICK_POWER_MEDIUM as i32,
    /// <= 100%
    Full = SDL_JOYSTICK_POWER_FULL as i32,
    Wired = SDL_JOYSTICK_POWER_WIRED as i32,
    Max = SDL_JOYSTICK_POWER_MAX as i32,
}
