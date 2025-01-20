use crate::bindings::SDL_Rect;

pub struct SdlRect {
    pub x: i32,
    pub y: i32,
    pub w: i32,
    pub h: i32,
}

impl From<SDL_Rect> for SdlRect {
    fn from(value: SDL_Rect) -> Self {
        Self {
            x: value.x,
            y: value.y,
            w: value.w,
            h: value.h,
        }
    }
}

impl From<SdlRect> for SDL_Rect {
    fn from(value: SdlRect) -> Self {
        Self {
            x: value.x,
            y: value.y,
            w: value.w,
            h: value.h,
        }
    }
}
