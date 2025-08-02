use std::collections::HashSet;
use std::fmt::Debug;
use std::hash::{DefaultHasher, Hash, Hasher};
use rstar::{PointDistance, RTreeObject, AABB};
use rstar::iterators::{RTreeIterator, RTreeIteratorMut};
use crate::math::Vector2f;

#[derive(Default)]
pub struct QuadTree<T: PartialEq + Eq + Copy + Hash> {
    tree: rstar::RTree<QuadTreeNode<T>>,
    hashes: HashSet<u64>,
}

impl<T: PartialEq + Eq + Copy + Hash> QuadTree<T> {
    pub fn new() -> Self {
        Self {
            tree: rstar::RTree::new(),
            hashes: HashSet::new(),
        }
    }
}

impl<T: PartialEq + Eq + Copy + Hash> QuadTree<T> {
    pub fn get_tree(&self) -> &rstar::RTree<QuadTreeNode<T>> {
        &self.tree
    }

    pub fn contains(&self, item: &T) -> bool {
        self.hashes.contains(&Self::hash_item(item))
    }

    pub fn insert(&mut self, item: QuadTreeNode<T>) {
        self.tree.insert(item);
        self.hashes.insert(Self::hash_item(&item.handle));
    }

    pub fn remove(&mut self, item: &T) {
        self.tree.remove(&QuadTreeNode::from_handle(*item));
        self.hashes.remove(&Self::hash_item(item));
    }

    fn hash_item(item: &T) -> u64 {
        let mut hasher = DefaultHasher::new();
        item.hash(&mut hasher);
        hasher.finish()
    }
}

impl<'a, T: PartialEq + Eq + Copy + Hash> IntoIterator for &'a QuadTree<T> {
    type Item = &'a QuadTreeNode<T>;
    type IntoIter = RTreeIterator<'a, QuadTreeNode<T>>;

    fn into_iter(self) -> Self::IntoIter {
        self.tree.iter()
    }
}

impl<'a, T: PartialEq + Eq + Copy + Hash> IntoIterator for &'a mut QuadTree<T> {
    type Item = &'a mut QuadTreeNode<T>;
    type IntoIter = RTreeIteratorMut<'a, QuadTreeNode<T>>;

    fn into_iter(self) -> Self::IntoIter {
        self.tree.iter_mut()
    }
}

/// A spatial point that wraps a handle and implements the necessary traits for rstar.
#[derive(Clone, Copy, Debug)]
pub struct QuadTreeNode<T: PartialEq + Eq + Copy + Hash> {
    /// The UUID of the object
    pub handle: T,
    /// The position of the object in 2D space
    pub min_extent: Vector2f,
    pub max_extent: Vector2f,
}

impl<T: PartialEq + Eq + Copy + Hash> QuadTreeNode<T> {
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
impl<T: PartialEq + Eq + Copy + Hash> PartialEq for QuadTreeNode<T> {
    fn eq(&self, other: &Self) -> bool {
        self.handle == other.handle
    }
}

impl<T: PartialEq + Eq + Copy + Hash> Eq for QuadTreeNode<T> {}

impl<T: PartialEq + Eq + Copy + Hash> RTreeObject for QuadTreeNode<T> {
    type Envelope = AABB<[f32; 2]>;

    fn envelope(&self) -> Self::Envelope {
        AABB::from_points(&[
            [self.min_extent.x, self.min_extent.y],
            [self.max_extent.x, self.max_extent.y],
        ])
    }
}

impl<T: PartialEq + Eq + Copy + Hash> PointDistance for QuadTreeNode<T> {
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
