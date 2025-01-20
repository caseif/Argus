use std::ffi::CString;
use std::{ffi, ptr};
use std::error::Error;
use std::fmt::{Display, Formatter};
use crate::bindings::*;
use crate::internal::util;
use crate::internal::util::c_str_to_string_lossy;

#[derive(Clone, Debug)]
pub struct SdlError {
    message: String,
}

impl Display for SdlError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "SDL reported error: {}", self.message)
    }
}

impl Error for SdlError {}

impl SdlError {
    pub(crate) fn new(message: impl Into<String>) -> SdlError {
        Self {
            message: message.into(),
        }
    }

    pub fn get_message(&self) -> &str {
        self.message.as_str()
    }
}

pub fn sdl_get_error() -> SdlError {
    unsafe {
        let message = c_str_to_string_lossy(SDL_GetError()).expect("SDL_GetError returned nullptr");
        SdlError { message }
    }
}

pub(crate) fn c_str_to_string_or_error(c_str: *const ffi::c_char) -> Result<String, SdlError> {
    c_str_to_string_lossy(c_str).ok_or_else(|| sdl_get_error())
}

pub(crate) fn wrap_or_error<P, W, F: FnOnce(*mut P) -> W>(raw_ptr: *mut P, f: F)
    -> Result<W, SdlError> {
    if raw_ptr.is_null() {
        return Err(sdl_get_error());
    }
    Ok(f(raw_ptr))
}
