use crate::math;
use rstar::iterators::RTreeIterator;
use rstar::{PointDistance, RTree, RTreeObject};
use std::collections::HashSet;
use std::fmt::Debug;
use std::hash::{DefaultHasher, Hash, Hasher};

#[derive(Default)]
pub struct QuadTree<T: PartialEq + Eq + Copy + Hash> {
    tree: RTree<QuadTreeNode<T>>,
    hashes: HashSet<u64>,
}

impl<T: PartialEq + Eq + Copy + Hash> QuadTree<T> {
    pub fn new() -> Self {
        Self {
            tree: RTree::new(),
            hashes: HashSet::new(),
        }
    }
}

impl<T: PartialEq + Eq + Copy + Hash> QuadTree<T> {
    pub fn get_tree(&self) -> &RTree<QuadTreeNode<T>> {
        &self.tree
    }

    pub fn contains(&self, item: &T) -> bool {
        self.hashes.contains(&Self::hash_item(item))
    }

    pub fn insert(&mut self, item: QuadTreeNode<T>) {
        self.tree.insert(item);
        self.hashes.insert(Self::hash_item(&item.handle));
    }
    
    pub fn update(&mut self, key: &T, aabb: math::AABB) {
        self.remove(key);
        self.insert(QuadTreeNode::new(*key, aabb));
    }

    pub fn remove(&mut self, item: &T) {
        self.tree.remove(&QuadTreeNode::from_handle(*item));
        self.hashes.remove(&Self::hash_item(item));
    }
    
    pub fn iter(&self) -> RTreeIterator<QuadTreeNode<T>> {
        (&self).into_iter()
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

/// A spatial point that wraps a handle and implements the necessary traits for rstar.
#[derive(Clone, Copy, Debug)]
pub struct QuadTreeNode<T: PartialEq + Eq + Copy + Hash> {
    /// The UUID of the object
    pub handle: T,
    /// The position of the object in 2D space
    pub aabb: math::AABB,
}

impl<T: PartialEq + Eq + Copy + Hash> QuadTreeNode<T> {
    pub fn new(handle: T, aabb: math::AABB) -> Self {
        Self {
            handle,
            aabb,
        }
    }

    pub fn from_handle(handle: T) -> Self {
        Self::new(handle, Default::default())
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
    type Envelope = rstar::AABB<[f32; 2]>;

    fn envelope(&self) -> Self::Envelope {
        self.aabb.into()
    }
}

impl<T: PartialEq + Eq + Copy + Hash> PointDistance for QuadTreeNode<T> {
    fn distance_2(&self, point: &[f32; 2]) -> f32 {
        // Calculate the closest point on the AABB to the given point
        let closest_x = point[0].max(self.aabb.get_min().x).min(self.aabb.get_max().x);
        let closest_y = point[1].max(self.aabb.get_min().y).min(self.aabb.get_max().y);

        // Calculate the squared distance from the closest point to the given point
        let dx = closest_x - point[0];
        let dy = closest_y - point[1];
        dx * dx + dy * dy
    }
}
