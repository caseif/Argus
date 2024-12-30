use std::ffi;
use std::ffi::CStr;

pub(crate) fn c_str_to_string_lossy(c_str: *const ffi::c_char) -> Option<String> {
    if c_str.is_null() {
        return None;
    }
    let c_str = unsafe { CStr::from_ptr(c_str) };
    Some(c_str.to_string_lossy().into_owned())
}
