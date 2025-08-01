use std::collections::HashMap;
use argus_resman::Resource;
use argus_util::dirtiable::{Dirtiable, ValueAndDirtyFlag};
use argus_util::math::{Vector2f, Vector3f};
use argus_util::pool::Handle;
use rstar::{RTree, AABB};
use argus_util::rtree::SpatialPoint;
use crate::common::*;
use crate::twod::*;

pub struct Scene2d {
    id: String,
    lighting_enabled: bool,
    ambient_light_level: Dirtiable<f32>,
    ambient_light_color: Dirtiable<Vector3f>,
    pub(crate) root_group_read: Option<Handle>,
    pub(crate) root_group_write: Option<Handle>,
    pub(crate) lights: Vec<Handle>,
    lights_quadtree: RTree<SpatialPoint<Handle>>,
    cameras: HashMap<String, Camera2d>,
    pub(crate) last_rendered_versions: HashMap<(SceneItemType, Handle), u16>,
}

impl Scene for Scene2d {
    #[must_use]
    fn get_type(&self) -> SceneType {
        SceneType::TwoDim
    }
}

impl Scene2d {
    #[must_use]
    pub(crate) fn new(id: impl Into<String>) -> Scene2d {
        let root_group_write = get_render_context_2d()
            .add_group(RenderGroup2d::new(Transform2d::default(), None));
        Self {
            id: id.into(),
            lighting_enabled: false,
            ambient_light_level: Dirtiable::new(1.0),
            ambient_light_color: Dirtiable::new(Vector3f::new(1.0, 1.0, 1.0)),
            root_group_read: None,
            root_group_write: Some(root_group_write),
            lights: Vec::new(),
            lights_quadtree: RTree::new(),
            cameras: HashMap::new(),
            last_rendered_versions: HashMap::new(),
        }
    }

    #[must_use]
    pub fn get_id(&self) -> &str {
        self.id.as_str()
    }

    #[must_use]
    pub fn is_lighting_enabled(&self) -> bool {
        self.lighting_enabled
    }

    pub fn set_lighting_enabled(&mut self, enabled: bool) {
        self.lighting_enabled = enabled;
    }

    #[must_use]
    pub fn peek_ambient_light_level(&self) -> f32 {
        self.ambient_light_level.peek().value
    }

    #[must_use]
    pub fn get_ambient_light_level(&mut self) -> ValueAndDirtyFlag<f32> {
        self.ambient_light_level.read()
    }

    pub fn set_ambient_light_level(&mut self, level: f32) {
        self.ambient_light_level.set(level);
    }

    #[must_use]
    pub fn peek_ambient_light_color(&self) -> Vector3f {
        self.ambient_light_color.peek().value
    }

    #[must_use]
    pub fn get_ambient_light_color(&mut self) -> ValueAndDirtyFlag<Vector3f> {
        self.ambient_light_color.read()
    }

    pub fn set_ambient_light_color(&mut self, color: Vector3f) {
        self.ambient_light_color.set(color);
    }

    pub fn get_light_handles(&self) -> &Vec<Handle> {
        &self.lights
    }

    pub fn add_light(
        &mut self,
        properties: Light2dProperties,
        initial_transform: Transform2d
    ) -> Handle {
        // Let there be light.
        // That's, uh... God. I was quoting God.
        let light = RenderLight2d::new(properties, initial_transform.clone());
        let context = get_render_context_2d();
        let handle = context.add_light(light);
        self.lights.push(handle);
        self.lights_quadtree.insert(
            SpatialPoint::new(handle, initial_transform.translation, initial_transform.translation),
        );
        handle
    }

    pub fn get_light(
        &mut self,
        handle: Handle
    ) -> Option<ContextObjectReadGuard<RenderLight2d>> {
        if !self.lights.contains(&handle) {
            return None;
        }
        let context = get_render_context_2d();
        context.get_light(handle)
    }

    pub fn get_light_mut(
        &mut self,
        handle: Handle
    ) -> Option<ContextObjectWriteGuard<RenderLight2d>> {
        if !self.lights.contains(&handle) {
            return None;
        }
        let context = get_render_context_2d();
        let light = context.get_light_mut(handle);

        light
    }

    pub fn update_light_position(&mut self, handle: Handle) {
        let context = get_render_context_2d();
        if self.lights.contains(&handle) {
            if let Some(light) = context.get_light(handle) {
                let position = light.get_transform().translation;
                self.lights_quadtree.insert(SpatialPoint::new(handle, position, position));
            }
        }
    }

    pub fn remove_light(&mut self, handle: Handle) -> bool {
        let context = get_render_context_2d();
        let result = context.remove_light(handle, self.id.as_str());
        if result {
            self.lights_quadtree.remove(&SpatialPoint {
                handle,
                // The extents don't matter for removal since we're using the uuid for equality
                min_extent: Default::default(),
                max_extent: Default::default(),
            });
            if let Some(index) = self.lights.iter().position(|&h| h == handle) {
                self.lights.swap_remove(index);
            }
        }
        result
    }

    pub fn get_group(&self, handle: Handle)
        -> Option<ContextObjectReadGuard<RenderGroup2d>> {
        let context = get_render_context_2d();
        let group = context.get_group(handle)?;
        if group.get_parent().is_some() {
            panic!("Tried to get group from scene which was not direct child of scene");
        }
        Some(group)
    }

    pub fn get_group_mut(&mut self, handle: Handle)
                     -> Option<ContextObjectWriteGuard<RenderGroup2d>> {
        let context = get_render_context_2d();
        let group = context.get_group_mut(handle)?;
        if group.get_parent().is_some() {
            panic!("Tried to get group from scene which was not direct child of scene");
        }
        Some(group)
    }

    pub fn get_object(&self, handle: Handle)
        -> Option<ContextObjectReadGuard<RenderObject2d>> {
        let context = get_render_context_2d();
        let object = context.get_object(handle)?;
        if object.get_parent() != self.root_group_write.unwrap() {
            panic!("Tried to get object from scene which was not direct child of scene");
        }
        Some(object)
    }

    pub fn get_object_mut(&mut self, handle: Handle)
        -> Option<ContextObjectWriteGuard<RenderObject2d>> {
        let context = get_render_context_2d();
        let object = context.get_object_mut(handle)?;
        if object.get_parent() != self.root_group_write.unwrap() {
            panic!("Tried to get object from scene which was not direct child of scene");
        }
        Some(object)
    }

    /// Creates a new group as a direct logical child of the scene.
    ///
    /// Internally, the object will be created as a child of the implicit root
    /// group of the Scene.
    ///
    /// Arguments:
    ///
    /// * `context` The current render context.
    /// * `transform` The local transform of the new group.
    pub fn add_group(&mut self, transform: Transform2d)
        -> Handle {
        let new_group = RenderGroup2d::new(transform, self.root_group_write);
        let context = get_render_context_2d();
        let new_handle = context.add_group(new_group);
        let mut root_group = context.get_group_mut(self.root_group_write.unwrap()).unwrap();
        root_group.child_groups.push(new_handle);
        new_handle
    }

    /// Creates a new [render object](RenderObject2d) as a direct logical child
    /// of the scene.
    ///
    /// Internally, the object will be created as a child of the implicit root
    /// group of the Scene so it will still technically have a parent group.
    ///
    /// Arguments:
    ///
    /// * `context` The current render context.
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
        material: Resource,
        primitives: Vec<RenderPrimitive2d>,
        anchor_point: Vector2f,
        atlas_stride: Vector2f,
        z_index: u32,
        light_opacity: f32,
        transform: Transform2d,
    ) -> Handle {
        let new_object = RenderObject2d::new(
            self.root_group_write.unwrap(),
            material,
            primitives,
            anchor_point,
            atlas_stride,
            z_index,
            light_opacity,
            transform,
        );
        let context = get_render_context_2d();
        let new_handle = context.add_object(new_object);
        let mut root_group = context.get_group_mut(self.root_group_write.unwrap()).unwrap();
        root_group.child_objects.push(new_handle);
        new_handle
    }

    /// Removes the child group with the given [Handle] from this one,
    /// destroying it in the process.
    pub fn remove_group(&mut self, handle: Handle) {
        let context = get_render_context_2d();
        if !context.remove_group(handle, Some(self.root_group_write.unwrap())) {
            panic!("Tried to remove group that doesn't exist in the current render context");
        }
    }

    /// Removes the child [object](RenderObject2d) with the given [Handle] from
    /// this group, destroying it in the process.
    pub fn remove_object(&mut self, handle: Handle) {
        let context = get_render_context_2d();
        if !context.remove_object(handle, self.root_group_write.unwrap()) {
            panic!("Tried to remove object that doesn't exist in the current render context");
        }
    }

    #[must_use]
    pub fn get_camera(&self, id: impl AsRef<str>) -> Option<&Camera2d> {
        self.cameras.get(id.as_ref())
    }

    #[must_use]
    pub fn get_camera_mut(&mut self, id: impl AsRef<str>) -> Option<&mut Camera2d> {
        self.cameras.get_mut(id.as_ref())
    }

    pub fn create_camera(&mut self, id: impl Into<String>) -> &mut Camera2d {
        let id_str = id.into();
        if self.cameras.contains_key(&id_str) {
            panic!("Tried to add camera with duplicate ID"); //TODO
        }

        self.cameras.entry(id_str.clone()).or_insert_with(|| {
            Camera2d::new(id_str.clone(), self.id.clone(), Transform2d::default())
        })
    }

    pub fn destroy_camera(&mut self, id: impl AsRef<str>) {
        self.cameras.remove(id.as_ref());
    }

    #[allow(unused)]
    pub(crate) fn get_lights_for_render(&self) -> Vec<RenderLight2d> {
        todo!()
    }

    pub fn get_lights_for_frustum(&self, viewport: &AttachedViewport2d, buffer: f32)
        -> Vec<Handle> {
        let frustum = viewport.get_view_frustum();
        let aabb = AABB::from_corners(
            [frustum.left - buffer, frustum.top - buffer],
            [frustum.right + buffer, frustum.bottom + buffer],
        );
        self.lights_quadtree.locate_in_envelope(&aabb)
            .map(|sp| sp.handle)
            .collect()
    }
}
