use std::sync::{Arc, Mutex};
use std::sync::atomic::{AtomicU16, Ordering};
use lazy_static::lazy_static;
use crate::common::Transform2d;
use crate::twod::{RenderContext2d, RenderObject2d, RenderPrimitive2d};
use lowlevel_rs::{Handle, ValuePool};
use lowlevel_rustabi::argus::lowlevel::{Dirtiable, ValueAndDirtyFlag, Vector2f};

lazy_static! {
    pub(crate) static ref RENDER_GROUP_POOL: Arc<Mutex<ValuePool<RenderGroup2d>>> =
        Arc::new(Mutex::new(ValuePool::new()));
}

/// Represents a set of groups and [objects](RenderObject2d) to be rendered together.
///
/// Each render group supplies a local transform which will be applied when
/// rendering children in addition to their own respective local transform.
pub struct RenderGroup2d {
    pub(crate) handle: Option<Handle>,
    transform: Dirtiable<Transform2d>,
    parent_group: Option<Handle>,
    pub(crate) child_groups: Vec<Handle>,
    pub(crate) child_objects: Vec<Handle>,
    pub(crate) version: Arc<AtomicU16>,
}

impl RenderGroup2d {
    /// Constructs a new render group, optionally as a child of another group.
    #[must_use]
    pub(crate) fn new(transform: Transform2d, parent_group: Option<Handle>) -> Self {
        Self {
            handle: None,
            transform: Dirtiable::new(transform),
            parent_group,
            child_groups: Vec::new(),
            child_objects: Vec::new(),
            version: Arc::new(AtomicU16::new(0)),
        }
    }

    /// Gets the parent group if one exists.
    #[must_use]
    pub fn get_parent(&self) -> Option<Handle> {
        self.parent_group
    }

    /// Creates a new group as a child of this one with the given local
    /// transform.
    pub fn add_group(&mut self, context: &mut RenderContext2d, transform: Transform2d) -> Handle {
        let child = RenderGroup2d::new(transform, self.handle);
        let handle = context.add_group(child);
        self.child_groups.push(handle);
        handle
    }

    /// Creates a new [render object](RenderObject2d) a child of this group.
    ///
    /// Arguments:
    ///
    /// * `material` The Material to be used by the new object.
    /// * `primitives` The list of [primitives](RenderPrim2d) comprising the new
    ///                object.
    /// * `anchor_point` The anchor point in local space about which
    ///                  transformations will be applied.
    /// * `atlas_stride` The size of tiles in the associated texture atlas. This
    ///                  is relevant in this context because it is used to apply
    ///                  sprite animations. See also:
    ///                  [RenderObject2d::get_atlas_stride]
    /// * `z_index` The z-index of the object within its parent. See also:
    ///             [RenderObject2d::get_z_index]
    /// * `light_opacity` The opacity of the object with respect to lighting.
    ///                   See also: [RenderObject2d::get_light_opacity]
    /// * `transform` The local transform of the new object.
    pub fn add_object(
        &mut self,
        context: &mut RenderContext2d,
        material: impl AsRef<str>,
        primitives: Vec<RenderPrimitive2d>,
        anchor_point: Vector2f,
        atlas_stride: Vector2f,
        z_index: u32,
        light_opacity: f32,
        transform: Transform2d,
    ) -> Handle {
        let object = RenderObject2d::new(
            self.handle.unwrap(),
            material.as_ref(),
            primitives,
            anchor_point,
            atlas_stride,
            z_index,
            light_opacity,
            transform,
        );
        let handle = context.add_object(object);
        self.child_objects.push(handle);
        handle
    }

    /// Removes the child group with the given [Handle] from this one,
    /// destroying it in the process.
    pub fn remove_group(&mut self, context: &mut RenderContext2d, handle: Handle) {
        if !context.remove_group(handle, self.handle) {
            panic!("Tried to remove group that doesn't exist in the current render context");
        }
    }

    /// Removes the child [object](RenderObject2d) with the given [Handle] from
    /// this group, destroying it in the process.
    pub fn remove_object(&mut self, context: &mut RenderContext2d, handle: Handle) {
        if !context.remove_group(handle, self.handle) {
            panic!("Tried to remove object that doesn't exist in the current render context");
        }
    }

    /// Peeks the local [transform](Transform2d) of this group without clearing
    /// its dirty flag.
    ///
    /// The returned transform is local and, if this group is a child of
    /// another, does not necessarily reflect the group's absolute transform
    /// with respect to its containing [scene](Scene2d).
    pub fn peek_transform(&self) -> Transform2d {
        self.transform.peek().value
    }

    /// Gets the local [transform](Transform2d) of this group, clearing its
    /// dirty flag in the process.
    ///
    /// The returned [Transform2d] is local and, if this group is a child of
    /// another, does not necessarily reflect the group's absolute transform
    /// with respect to its containing [scene](Scene2d).
    #[must_use]
    pub fn get_transform(&mut self) -> ValueAndDirtyFlag<Transform2d> {
        self.transform.read()
    }

    /// Sets the local [transform](Transform2d) of this group.
    ///
    /// The local transform does not necessarily reflect the group's absolute
    /// transform with respect to its containing scene, which is computed from
    /// the full hierarchy of groups.
    pub fn set_transform(&mut self, transform: Transform2d) {
        if transform == self.transform.peek().value {
            return;
        }
        self.transform.set(transform);
        self.version.fetch_add(1, Ordering::Relaxed);
    }

    #[must_use]
    pub fn duplicate(&self, _context: &mut RenderContext2d) -> RenderGroup2d {
        todo!()
    }
}
