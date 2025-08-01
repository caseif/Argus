use std::hash::Hash;
use rstar::{PointDistance, RTreeObject, AABB};
use crate::math::Vector2f;

/// A spatial point that wraps a handle and implements the necessary traits for rstar.
#[derive(Clone, Copy, Debug)]
pub struct SpatialPoint<T: PartialEq + Eq + Copy + Hash> {
    /// The UUID of the object
    pub handle: T,
    /// The position of the object in 2D space
    pub min_extent: Vector2f,
    pub max_extent: Vector2f,
}

impl<T: PartialEq + Eq + Copy + Hash> SpatialPoint<T> {
    pub fn new(handle: T, min_extent: Vector2f, max_extent: Vector2f) -> Self {
        Self {
            handle,
            min_extent,
            max_extent,
        }
    }

    pub fn from_handle(handle: T) -> Self {
        Self::new(handle, Default::default(), Default::default())
    }
}

// Implement PartialEq and Eq to compare only by handle
impl<T: PartialEq + Eq + Copy + Hash> PartialEq for SpatialPoint<T> {
    fn eq(&self, other: &Self) -> bool {
        self.handle == other.handle
    }
}

impl<T: PartialEq + Eq + Copy + Hash> Eq for SpatialPoint<T> {}

impl<T: PartialEq + Eq + Copy + Hash> RTreeObject for SpatialPoint<T> {
    type Envelope = AABB<[f32; 2]>;

    fn envelope(&self) -> Self::Envelope {
        AABB::from_points(&[
            [self.min_extent.x, self.min_extent.y],
            [self.max_extent.x, self.max_extent.y],
        ])
    }
}

impl<T: PartialEq + Eq + Copy + Hash> PointDistance for SpatialPoint<T> {
    fn distance_2(&self, point: &[f32; 2]) -> f32 {
        // Calculate the closest point on the AABB to the given point
        let closest_x = point[0].max(self.min_extent.x).min(self.max_extent.x);
        let closest_y = point[1].max(self.min_extent.y).min(self.max_extent.y);

        // Calculate the squared distance from the closest point to the given point
        let dx = closest_x - point[0];
        let dy = closest_y - point[1];
        dx * dx + dy * dy
    }
}
