use std::ops::Mul;
use crate::math::{Vector2f, Vector4f};
use crate::math::Matrix4x4;

#[derive(Clone, Copy, Debug, Default)]
pub struct AABB {
    min: Vector2f,
    max: Vector2f,
}

impl AABB {
    pub fn from_corners(a: Vector2f, b: Vector2f) -> Self {
        Self {
            min: Vector2f::new(a.x.min(b.x), a.y.min(b.y)),
            max: Vector2f::new(a.x.max(b.x), a.y.max(b.y)),
        }
    }
    pub fn from_point(point: Vector2f) -> Self {
        Self {
            min: point,
            max: point,
        }
    }

    pub fn get_min(&self) -> &Vector2f {
        &self.min
    }

    pub fn get_max(&self) -> &Vector2f {
        &self.max
    }

    pub fn intersects(&self, other: &Self) -> bool {
        self.max.x >= other.min.x && self.min.x <= other.max.x &&
            self.max.y >= other.min.y && self.min.y <= other.max.y
    }

    pub fn expand(&self, buffer: f32) -> Self {
        Self {
            min: self.min - (buffer, buffer),
            max: self.max + (buffer, buffer),
        }
    }
}

impl Into<rstar::AABB<[f32; 2]>> for AABB {
    fn into(self) -> rstar::AABB<[f32; 2]> {
        rstar::AABB::from_corners(self.min.into(), self.max.into())
    }
}

impl Into<rstar::AABB<[f32; 2]>> for &AABB {
    fn into(self) -> rstar::AABB<[f32; 2]> {
        self.clone().into()
    }
}

impl Mul<AABB> for &Matrix4x4 {
    type Output = AABB;

    fn mul(self, rhs: AABB) -> Self::Output {
        let v0 = self * Vector4f::new(rhs.min.x, rhs.min.y, 0.0, 1.0);
        let v1 = self * Vector4f::new(rhs.max.x, rhs.min.y, 0.0, 1.0);
        let v2 = self * Vector4f::new(rhs.max.x, rhs.max.y, 0.0, 1.0);
        let v3 = self * Vector4f::new(rhs.min.x, rhs.max.y, 0.0, 1.0);
        let min_x = v0.x.min(v1.x.min(v2.x.min(v3.x)));
        let max_x = v0.x.max(v1.x.max(v2.x.max(v3.x)));
        let min_y = v0.y.min(v1.y.min(v2.y.min(v3.y)));
        let max_y = v0.y.max(v1.y.max(v2.y.max(v3.y)));
        AABB::from_corners(
            Vector2f::new(min_x, min_y),
            Vector2f::new(max_x, max_y),
        )
    }
}

impl Mul<AABB> for Matrix4x4 {
    type Output = AABB;

    fn mul(self, rhs: AABB) -> Self::Output {
        &self * rhs
    }
}
