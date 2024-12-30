use std::ffi;
use crate::bindings::{SDL_GetWindowFromID, SDL_Window};
use crate::error::{sdl_get_error, SdlError};

pub struct SdlWindow {
    handle: *mut SDL_Window,
}

impl SdlWindow {
    pub fn from_id(id: u32) -> Result<Self, SdlError> {
        let window_ptr = unsafe { SDL_GetWindowFromID(id) };
        if window_ptr.is_null() {
            return Err(sdl_get_error());
        }
        Ok(Self { handle: window_ptr })
    }

    pub fn get_handle(&self) -> *const ffi::c_void {
        self.handle.cast()
    }

    pub fn get_handle_mut(&mut self) -> *mut ffi::c_void {
        self.handle.cast()
    }
}
