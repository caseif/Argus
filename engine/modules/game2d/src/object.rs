use box2d_sys::b2BodyId;
use argus_util::math::Vector2f;
use argus_util::pool::Handle;
use crate::sprite::Sprite;

pub struct CommonObjectProperties {
    pub(crate) size: Vector2f,
    pub(crate) z_index: u32,
    pub(crate) collision_layer: u64,
    pub(crate) collision_mask: u64,
    pub(crate) sprite: Sprite,

    pub(crate) b2_body: Option<b2BodyId>,

    pub(crate) render_obj: Option<Handle>,
}
