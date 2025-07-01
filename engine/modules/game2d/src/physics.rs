use argus_scripting_bind::script_bind;
use argus_util::math::Vector2f;

#[derive(Clone, Copy, Debug)]
pub enum BoundingShape {
    Rectangle(BoundingRectangle),
    Capsule(BoundingCapsule),
    Circle(BoundingCircle),
}

#[derive(Clone, Copy, Debug)]
#[script_bind]
pub struct BoundingRectangle {
    pub size: Vector2f,
    pub rotation: f32,
}

#[derive(Clone, Copy, Debug)]
#[script_bind]
pub struct BoundingCapsule {
    pub width: f32,
    pub length: f32,
    pub rotation: f32,
    pub offset: Vector2f,
}

#[derive(Clone, Copy, Debug)]
#[script_bind]
pub struct BoundingCircle {
    pub radius: f32,
}
