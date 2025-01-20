use std::ffi;
use std::ffi::CStr;

pub(crate) const fn trim_null_terminator(s: &[u8]) -> &[u8] {
    if let [rest @ .., last] = s {
        if *last == 0 {
            return rest;
        }
    }
    s
}

pub(crate) const unsafe fn from_c_define(str_def: &[u8]) -> &str {
    unsafe { std::str::from_utf8_unchecked(trim_null_terminator(str_def)) }
}

pub(crate) fn c_str_to_string_lossy(c_str: *const ffi::c_char) -> Option<String> {
    if c_str.is_null() {
        return None;
    }
    let c_str = unsafe { CStr::from_ptr(c_str) };
    Some(c_str.to_string_lossy().into_owned())
}
