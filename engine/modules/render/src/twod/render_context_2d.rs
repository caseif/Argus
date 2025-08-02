use crate::twod::{RenderGroup2d, RenderLight2d, RenderObject2d, Scene2d};
use argus_util::pool::{Handle, ValuePool};
use dashmap::mapref::one::{Ref, RefMut};
use dashmap::DashMap;
use lazy_static::lazy_static;
use parking_lot::{MappedRwLockReadGuard, MappedRwLockWriteGuard, RwLock, RwLockReadGuard, RwLockWriteGuard};
use std::ops::{Deref, DerefMut};

lazy_static! {
    static ref RENDER_CONTEXT: RenderContext2d = RenderContext2d::new();
}

#[derive(Default)]
pub struct RenderContext2d {
    scenes: DashMap<String, Scene2d>,
    group_pool: RwLock<ValuePool<RenderGroup2d>>,
    object_pool: RwLock<ValuePool<RenderObject2d>>,
    light_pool: RwLock<ValuePool<RenderLight2d>>,
}

pub struct ContextObjectReadGuard<'a, T> {
    guard: MappedRwLockReadGuard<'a, T>,
}

impl<T> Deref for ContextObjectReadGuard<'_, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        self.guard.deref()
    }
}

impl<'a, T> From<MappedRwLockReadGuard<'a, T>> for ContextObjectReadGuard<'a, T> {
    fn from(guard: MappedRwLockReadGuard<'a, T>) -> Self {
        Self { guard }
    }
}

pub struct ContextObjectWriteGuard<'a, T> {
    guard: MappedRwLockWriteGuard<'a, T>,
}

impl<T> Deref for ContextObjectWriteGuard<'_, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        self.guard.deref()
    }
}

impl<T> DerefMut for ContextObjectWriteGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.guard.deref_mut()
    }
}

impl<'a, T> From<MappedRwLockWriteGuard<'a, T>> for ContextObjectWriteGuard<'a, T> {
    fn from(guard: MappedRwLockWriteGuard<'a, T>) -> Self {
        Self { guard }
    }
}

pub struct ContextObjectRef<'a, T> {
    guard: Ref<'a, String, T>,
}

impl<T> Deref for ContextObjectRef<'_, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        self.guard.deref()
    }
}

impl<'a, T> From<Ref<'a, String, T>> for ContextObjectRef<'a, T> {
    fn from(guard: Ref<'a, String, T>) -> Self {
        Self { guard }
    }
}

pub struct ContextObjectRefMut<'a, T> {
    guard: RefMut<'a, String, T>,
}

impl<T> Deref for ContextObjectRefMut<'_, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        self.guard.deref()
    }
}

impl<T> DerefMut for ContextObjectRefMut<'_, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.guard.deref_mut()
    }
}

#[inline(always)]
fn get_pool_value<T>(pool: &RwLock<ValuePool<T>>, handle: Handle)
    -> Option<ContextObjectReadGuard<T>> {
    RwLockReadGuard::try_map(pool.read(), |p| p.get(handle))
        .ok()
        .map(Into::into)
}

#[inline(always)]
fn get_pool_value_mut<T>(pool: &RwLock<ValuePool<T>>, handle: Handle)
                     -> Option<ContextObjectWriteGuard<T>> {
    RwLockWriteGuard::try_map(pool.write(), |p| p.get_mut(handle))
        .ok()
        .map(Into::into)
}

pub fn get_render_context_2d() -> &'static RenderContext2d {
    &RENDER_CONTEXT
}

impl RenderContext2d {
    pub fn new() -> Self {
        Self {
            scenes: DashMap::new(),
            group_pool: RwLock::new(ValuePool::new()),
            object_pool: RwLock::new(ValuePool::new()),
            light_pool: RwLock::new(ValuePool::new()),
        }
    }

    pub fn get_scene(&self, scene_id: impl AsRef<str>) -> Option<Ref<String, Scene2d>> {
        self.scenes.get(scene_id.as_ref())
    }

    pub fn get_scene_mut(&self, scene_id: impl AsRef<str>)
        -> Option<RefMut<String, Scene2d>> {
        self.scenes.get_mut(scene_id.as_ref())
    }

    pub fn create_scene(&self, id: impl Into<String>)
        -> RefMut<String, Scene2d> {
        let id = id.into();
        self.scenes
            .entry(id.clone())
            .or_insert_with(|| Scene2d::new(id))
    }

    pub fn remove_scene(&self, id: impl Into<String>) -> bool {
        let Some((_, scene)) = self.scenes.remove(&id.into()) else { return false; };

        if let Some(root_handle) = scene.root_group_read {
            self.remove_group(root_handle, None);
        }

        if let Some(root_handle) = scene.root_group_write {
            self.remove_group(root_handle, None);
        }

        true
    }

    pub(crate) fn add_group(&self, group: RenderGroup2d) -> Handle {
        self.group_pool.write().insert_and_then(
            group,
            |inserted, handle| {
                inserted.handle = Some(handle);
            }
        )
    }

    pub(crate) fn remove_group(&self, handle: Handle, parent: Option<Handle>) -> bool {
        let Some(base_group) = self.group_pool.write().remove(handle) else { return false; };
        if base_group.get_parent() != parent {
            panic!("Tried to remove group from incorrect parent");
        }
        let mut child_groups = base_group.child_groups.clone();
        while let Some(child_group_handle) = child_groups.pop() {
            let child_group = self.group_pool.write().remove(child_group_handle).unwrap();
            child_groups.extend(child_group.child_groups);

            for child_object_handle in child_group.child_objects {
                self.remove_object(child_object_handle, child_group_handle);
            }
        }
        if let Some(parent_handle) = parent {
            self.group_pool.write().get_mut(parent_handle)
                .expect("Group parent was missing from context!")
                .child_groups
                .retain(|g| g != &handle);
        }
        true
    }

    pub(crate) fn get_group(&self, handle: Handle)
        -> Option<ContextObjectReadGuard<RenderGroup2d>> {
        get_pool_value(&self.group_pool, handle)
    }

    pub(crate) fn get_group_mut(&self, handle: Handle)
        -> Option<ContextObjectWriteGuard<RenderGroup2d>> {
        get_pool_value_mut(&self.group_pool, handle)
    }

    pub(crate) fn add_object(&self, object: RenderObject2d) -> Handle {
        self.object_pool.write().insert_and_then(
            object,
            |inserted, handle| {
                inserted.handle = Some(handle);
            }
        )
    }

    pub(crate) fn remove_object(&self, handle: Handle, parent: Handle) -> bool {
        let Some(object) = self.object_pool.write().remove(handle) else { return false; };
        if object.get_parent() != parent {
            panic!("Tried to remove object from incorrect parent");
        }
        let mut group_pool = self.group_pool.write();
        let parent_group = group_pool.get_mut(object.get_parent())
            .expect("Object parent was missing from context!");
        parent_group.child_objects.retain(|obj| obj != &handle);
        true
    }

    pub fn get_object(&self, handle: Handle) -> Option<ContextObjectReadGuard<RenderObject2d>> {
        get_pool_value(&self.object_pool, handle)
    }

    pub fn get_object_mut(&self, handle: Handle)
        -> Option<ContextObjectWriteGuard<RenderObject2d>> {
        get_pool_value_mut(&self.object_pool, handle)
    }

    pub(crate) fn add_light(&self, light: RenderLight2d) -> Handle {
        self.light_pool.write().insert(light)
    }

    pub(crate) fn remove_light(&self, handle: Handle) -> bool {
        self.light_pool.write().remove(handle).is_some()
    }

    pub(crate) fn get_light(&self, handle: Handle) -> Option<ContextObjectReadGuard<RenderLight2d>> {
        get_pool_value(&self.light_pool, handle)
    }

    pub(crate) fn get_light_mut(&self, handle: Handle) -> Option<ContextObjectWriteGuard<RenderLight2d>> {
        get_pool_value_mut(&self.light_pool, handle)
    }
}
