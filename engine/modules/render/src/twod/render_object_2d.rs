use std::sync::Arc;
use std::sync::atomic::{AtomicU16, Ordering};
use argus_resman::{Resource, WeakResource};
use argus_util::dirtiable::{Dirtiable, ValueAndDirtyFlag};
use argus_util::math::{Vector2f, Vector2u};
use argus_util::pool::Handle;
use crate::common::Transform2d;
use crate::twod::RenderPrimitive2d;

pub struct RenderObject2d {
    pub(crate) handle: Option<Handle>,
    parent_group: Handle,
    material: Resource,
    primitives: Vec<RenderPrimitive2d>,
    anchor_point: Vector2f,
    atlas_stride: Vector2f,
    z_index: u32,
    light_opacity: Dirtiable<f32>,
    transform: Dirtiable<Transform2d>,
    active_frame: Dirtiable<Vector2u>,
    pub(crate) version: Arc<AtomicU16>,
}

impl RenderObject2d {
    #[must_use]
    pub(crate) fn new(
        parent_group: Handle,
        material: Resource,
        primitives: Vec<RenderPrimitive2d>,
        anchor_point: Vector2f,
        atlas_stride: Vector2f,
        z_index: u32,
        light_opacity: f32,
        transform: Transform2d,
    ) -> RenderObject2d {
        Self {
            handle: None,
            parent_group,
            material,
            primitives,
            anchor_point,
            atlas_stride,
            z_index,
            light_opacity: Dirtiable::new(light_opacity),
            transform: Dirtiable::new(transform),
            active_frame: Default::default(),
            version: Arc::new(AtomicU16::new(0)),
        }
    }

    pub fn get_handle(&self) -> Option<Handle> {
        self.handle
    }

    /// Gets the parent group of this object.
    #[must_use]
    pub fn get_parent(&self) -> Handle {
        self.parent_group
    }

    /// Gets the material used by the object.
    #[must_use]
    pub fn get_material(&self) -> WeakResource {
        self.material.downgrade()
    }

    /// Gets the list of [primitives](RenderPrimitive2d) comprising this object.
    #[must_use]
    pub fn get_primitives(&self) -> &Vec<RenderPrimitive2d> {
        &self.primitives
    }

    /// Gets the anchor point of the object about which rotation and scaling
    /// will be applied.
    #[must_use]
    pub fn get_anchor_point(&self) -> Vector2f {
        self.anchor_point
    }

    /// Gets the stride on each axis between tiles in the texture atlas, if the
    /// object has an animated texture.
    ///
    /// The stride between atlas tiles.
    #[must_use]
    pub fn get_atlas_stride(&self) -> Vector2f {
        self.atlas_stride
    }

    /// Gets the z-index of the object. Objects with larger z-indices will be
    /// rendered in front of lower-indexed ones.
    #[must_use]
    pub fn get_z_index(&self) -> u32 {
        self.z_index
    }

    /// Gets the opacity of the object with respect to lighting.
    ///
    /// 0.0 indicates an object which light will fully pass through while 1.0
    /// indicates an object which no light will pass through.
    ///
    /// In practice this may be treated as a binary setting where values over
    /// a certain threshold are treated as opaque and values under are
    /// treated as translucent.
    #[must_use]
    pub fn peek_light_opacity(&self) -> f32 {
        self.light_opacity.peek().value
    }

    /// Sets the opacity of the object with respect to lighting.
    ///
    /// 0.0 indicates an object which light will fully pass through while 1.0
    /// indicates an object which no light will pass through.
    ///
    /// In practice this may be treated as a binary setting where values over
    /// a certain threshold are treated as opaque and values under are
    /// treated as translucent.
    pub fn set_light_opacity(&mut self, opacity: f32) {
        self.light_opacity.set(opacity);
    }

    /// Gets the active animation frame.
    ///
    /// The returned value is an x- and y-offset into the associated texture
    /// atlas.
    #[must_use]
    pub fn get_active_frame(&mut self) -> ValueAndDirtyFlag<Vector2u> {
        self.active_frame.read()
    }

    /// Sets the active animation frame.
    ///
    /// This is represented as an x- and y-offset into the associated texture
    /// atlas. Neither index should exceed the number of tiles in each
    /// dimension in the atlas.
    pub fn set_active_frame(&mut self, frame: Vector2u) {
        self.active_frame.set(frame);
    }

    /// Peeks the local transform of this object without clearing its dirty
    /// flag.
    ///
    /// The returned [Transform2d] is local and does not necessarily reflect
    /// the object's absolute transform with respect to the Scene containing
    /// it.
    #[must_use]
    pub fn peek_transform(&self) -> Transform2d {
        self.transform.peek().value
    }

    /// Gets the local [transform](Transform2d) of this object, clearing its
    /// dirty flag in the process.
    ///
    /// The returned [Transform2d] is local and does not necessarily reflect
    /// the object's absolute transform with respect to the Scene containing
    /// it.
    #[must_use]
    pub fn get_transform(&mut self) -> ValueAndDirtyFlag<Transform2d> {
        self.transform.read()
    }

    /// Sets the local Transform of this object.
    ///
    /// The object's transform is local and does not necessarily reflect its
    /// absolute transform with respect to the scene containing it.
    pub fn set_transform(&mut self, transform: Transform2d) {
        if transform == self.transform.peek().value {
            return;
        }
        self.transform.set(transform);
        self.version.fetch_add(1, Ordering::Relaxed);
    }

    #[must_use]
    pub fn duplicate(&self, _parent: Handle) -> Handle {
        todo!()
    }
}
