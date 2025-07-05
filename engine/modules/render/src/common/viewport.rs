use std::sync::atomic::{AtomicU32, Ordering};
use lazy_static::lazy_static;
use argus_util::math::Vector2f;

lazy_static! {
    pub(crate) static ref NEXT_VIEWPORT_ID: AtomicU32 = AtomicU32::new(1);
}

#[derive(Clone, Copy, Debug)]
pub struct Viewport {
    pub top: f32,
    pub bottom: f32,
    pub left: f32,
    pub right: f32,
    pub scaling: Vector2f,
    pub mode: ViewportCoordinateSpaceMode,
}

/// Specifies the conversion from [`Viewport`] coordinate space to pixel space for
/// each axis with respect to surface aspect ratio.
///
/// This can be conceptualized as defining where the values of 0 and 1 are
/// placed on the physical surface for each axis.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ViewportCoordinateSpaceMode {
    /// Each axis will be scaled independently, with 0 and 1 being on opposite
    /// edges of the surface.
    Individual,
    /// Both axes will be scaled relative to the smaller of the two axes..
    ///
    /// If the axes are not equal in length, the edges of the viewport on the
    /// larger dimension will stop short of the edges of the surface.
    MinAxis,
    /// Both axes will be scaled relative to the larger of the two axes.
    /// 
    /// If the axes are not equal in length, the edges of the viewport on the
    /// smaller dimension will fall outside of the bounds of the surface.
    MaxAxis,
    /// Both axes will be scaled relative to the horizontal axis regardless of
    /// which dimension is larger.
    HorizontalAxis,
    /// Both axes will be scaled relative to the vertical axis regardless of
    /// which dimension is larger.
    VerticalAxis
}

pub trait AttachedViewport {
    #[must_use]
    fn get_id(&self) -> u32;

    #[must_use]
    fn get_viewport(&self) -> &Viewport;

    #[must_use]
    fn get_z_index(&self) -> u32;

    #[must_use]
    fn get_postprocessing_shaders(&self) -> &Vec<String>;

    fn add_postprocessing_shader(&mut self, shader_uid: String);

    fn remove_postprocessing_shader(&mut self, shader_uid: String);
}

#[must_use]
pub(crate) fn get_next_viewport_id() -> u32 {
    let id = NEXT_VIEWPORT_ID.fetch_add(1, Ordering::Relaxed);
    if id == u32::MAX {
        panic!("Exhausted viewport IDs");
    }
    id
}
