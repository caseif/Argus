use num_enum::{IntoPrimitive, TryFromPrimitive, UnsafeFromPrimitive};
use crate::bindings::*;
use crate::error::{c_str_to_string_or_error, sdl_get_error, wrap_or_error, SdlError};
use crate::joystick::SdlJoystick;

#[allow(clippy::unnecessary_cast)]
#[repr(i32)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive, UnsafeFromPrimitive)]
pub enum SdlGameControllerButton {
    Invalid = SDL_CONTROLLER_BUTTON_INVALID as i32,
    A = SDL_CONTROLLER_BUTTON_A as i32,
    B = SDL_CONTROLLER_BUTTON_B as i32,
    X = SDL_CONTROLLER_BUTTON_X as i32,
    Y = SDL_CONTROLLER_BUTTON_Y as i32,
    Back = SDL_CONTROLLER_BUTTON_BACK as i32,
    Guide = SDL_CONTROLLER_BUTTON_GUIDE as i32,
    Start = SDL_CONTROLLER_BUTTON_START as i32,
    LeftStick = SDL_CONTROLLER_BUTTON_LEFTSTICK as i32,
    RightStick = SDL_CONTROLLER_BUTTON_RIGHTSTICK as i32,
    LeftShoulder = SDL_CONTROLLER_BUTTON_LEFTSHOULDER as i32,
    RightShoulder = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER as i32,
    DpadUp = SDL_CONTROLLER_BUTTON_DPAD_UP as i32,
    DpadDown = SDL_CONTROLLER_BUTTON_DPAD_DOWN as i32,
    DpadLeft = SDL_CONTROLLER_BUTTON_DPAD_LEFT as i32,
    DpadRight = SDL_CONTROLLER_BUTTON_DPAD_RIGHT as i32,
    Misc1 = SDL_CONTROLLER_BUTTON_MISC1 as i32,
    Paddle1 = SDL_CONTROLLER_BUTTON_PADDLE1 as i32,
    Paddle2 = SDL_CONTROLLER_BUTTON_PADDLE2 as i32,
    Paddle3 = SDL_CONTROLLER_BUTTON_PADDLE3 as i32,
    Paddle4 = SDL_CONTROLLER_BUTTON_PADDLE4 as i32,
    Touchpad = SDL_CONTROLLER_BUTTON_TOUCHPAD as i32,
    MaxValue = SDL_CONTROLLER_BUTTON_MAX as i32,
}

#[allow(clippy::unnecessary_cast)]
#[repr(i32)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive, UnsafeFromPrimitive)]
pub enum SdlGameControllerAxis {
    Invalid = SDL_CONTROLLER_AXIS_INVALID as i32,
    LeftX = SDL_CONTROLLER_AXIS_LEFTX as i32,
    LeftY = SDL_CONTROLLER_AXIS_LEFTY as i32,
    RightX = SDL_CONTROLLER_AXIS_RIGHTX as i32,
    RightY = SDL_CONTROLLER_AXIS_RIGHTY as i32,
    TriggerLeft = SDL_CONTROLLER_AXIS_TRIGGERLEFT as i32,
    TriggerRight = SDL_CONTROLLER_AXIS_TRIGGERRIGHT as i32,
    MaxValue = SDL_CONTROLLER_AXIS_MAX as i32,
}

#[derive(Clone)]
pub struct SdlGameController {
    handle: *mut SDL_GameController,
}

impl SdlGameController {
    fn of(handle: *mut SDL_GameController) -> Option<Self> {
        if handle.is_null() {
            return None;
        }
        Some(Self { handle })
    }

    pub fn open(joystick_index: i32) -> Result<SdlGameController, SdlError> {
        wrap_or_error(
            unsafe { SDL_GameControllerOpen(joystick_index) },
            |handle| SdlGameController::of(handle).unwrap(),
        )
    }

    pub fn from_instance_id(instance_id: i32) -> Result<SdlGameController, SdlError> {
        wrap_or_error(
            unsafe { SDL_GameControllerFromInstanceID(instance_id) },
            |handle| SdlGameController::of(handle).unwrap(),
        )
    }
    
    pub fn close(self) {
        unsafe { SDL_GameControllerClose(self.handle) }
    }

    pub fn get_instance_id(&self) -> Result<i32, SdlError> {
        unsafe {
            let joystick = SDL_GameControllerGetJoystick(self.handle);
            if joystick.is_null() {
                return Err(sdl_get_error());
            }

            Ok(SDL_JoystickInstanceID(joystick))
        }
    }

    pub fn get_name(&self) -> Result<String, SdlError> {
        c_str_to_string_or_error(unsafe { SDL_GameControllerName(self.handle) })
    }

    pub fn is_button_pressed(&self, button: SdlGameControllerButton) -> bool {
        unsafe { SDL_GameControllerGetButton(self.handle, button.into()) != 0 }
    }

    pub fn get_axis_value(&self, axis: SdlGameControllerAxis) -> i16 {
        unsafe { SDL_GameControllerGetAxis(self.handle, axis.into()) }
    }
}
