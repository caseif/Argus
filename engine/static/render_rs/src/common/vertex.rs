use lowlevel_rustabi::argus::lowlevel::{Vector2f, Vector4f};

#[derive(Clone, Copy, Debug)]
#[must_use]
pub struct Vertex2d {
    pub position: Vector2f,
    pub normal: Vector2f,
    pub color: Vector4f,
    pub tex_coord: Vector2f,
}
