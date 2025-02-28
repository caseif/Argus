use std::any::TypeId;
use std::collections::HashSet;
use std::sync::atomic::{AtomicU32, AtomicU64};
use std::sync::{atomic, Arc, OnceLock};
use std::thread::ThreadId;
use std::time::Duration;
use fragile::Fragile;
use parking_lot::{Mutex, MutexGuard};
use crate::{LifecycleStage, ScreenSpaceScaleMode, TargetThread};
use crate::buffered_map::BufferedMap;
use crate::{ArgusEvent, Ordering};

static INSTANCE: OnceLock<EngineManager> = OnceLock::new();

pub(crate) struct EngineCallback<F: ?Sized> {
    pub(crate) f: Box<F>,
    pub(crate) ordering: Ordering,
}

impl<F: ?Sized> EngineCallback<F> {
    fn new(f: Box<F>, ordering: Ordering) -> Self {
        Self { f, ordering }
    }
}

#[derive(Default)]
pub struct EngineManager {
    cur_stage: AtomicU32,

    loaded_modules: Arc<Mutex<HashSet<String>>>,
    render_backends: Arc<Mutex<HashSet<String>>>,
    active_render_backend: OnceLock<String>,

    engine_config_staging: Fragile<Mutex<EngineConfig>>,
    engine_config: OnceLock<EngineConfig>,

    update_thread_id: OnceLock<ThreadId>,

    next_callback_index: AtomicU64,
    pub(crate) update_callbacks: BufferedMap<u64, EngineCallback<dyn Send + Fn(Duration)>>,
    pub(crate) render_callbacks: BufferedMap<u64, EngineCallback<dyn Send + Fn(Duration)>>,
    pub(crate) update_one_off_callbacks: BufferedMap<u64, Box<dyn Send + FnOnce()>>,
    pub(crate) render_one_off_callbacks: BufferedMap<u64, Box<dyn Send + FnOnce()>>,

    pub(crate) update_event_handlers: BufferedMap<u64, (EngineCallback<dyn Send + Fn(&Arc<dyn ArgusEvent>)>, TypeId)>,
    pub(crate) render_event_handlers: BufferedMap<u64, (EngineCallback<dyn Send + Fn(&Arc<dyn ArgusEvent>)>, TypeId)>,
    pub(crate) update_event_queue: Arc<Mutex<Vec<(Arc<dyn ArgusEvent>, TypeId)>>>,
    pub(crate) render_event_queue: Arc<Mutex<Vec<(Arc<dyn ArgusEvent>, TypeId)>>>,
}

#[derive(Clone, Debug, Default)]
pub struct EngineConfig {
    pub target_tickrate: Option<u32>,
    pub target_framerate: Option<u32>,
    pub load_modules: Vec<String>,
    pub preferred_render_backends: Vec<String>,
    pub screen_scale_mode: ScreenSpaceScaleMode,
}

impl EngineManager {
    pub fn instance() -> &'static Self {
        INSTANCE.get_or_init(Self::new)
    }

    fn new() -> Self {
        Self {
            cur_stage: AtomicU32::new(LifecycleStage::Load.into()),

            loaded_modules: Arc::default(),
            render_backends: Arc::default(),
            active_render_backend: OnceLock::default(),

            engine_config_staging: Fragile::default(),
            engine_config: OnceLock::new(),

            update_thread_id: OnceLock::new(),

            next_callback_index: AtomicU64::new(1),
            update_callbacks: BufferedMap::new(),
            render_callbacks: BufferedMap::new(),
            update_one_off_callbacks: BufferedMap::new(),
            render_one_off_callbacks: BufferedMap::new(),

            update_event_handlers: BufferedMap::new(),
            render_event_handlers: BufferedMap::new(),
            update_event_queue: Default::default(),
            render_event_queue: Default::default(),
        }
    }

    pub fn get_current_lifecycle_stage(&self) -> LifecycleStage {
        LifecycleStage::try_from(self.cur_stage.load(atomic::Ordering::Relaxed)).unwrap()
    }

    pub(crate) fn set_current_lifecycle_stage(&self, stage: LifecycleStage) {
        self.cur_stage.store(stage.into(), atomic::Ordering::Relaxed);
    }

    pub(crate) fn get_loaded_modules(&self) -> MutexGuard<HashSet<String>> {
        self.loaded_modules.lock()
    }

    pub(crate) fn get_available_render_backends(&self) -> MutexGuard<HashSet<String>> {
        self.render_backends.lock()
    }

    pub(crate) fn get_active_render_backend(&self) -> String {
        self.active_render_backend.get().cloned()
            .expect("Active render backend is not yet set")
    }

    pub(crate) fn set_active_render_backend(&self, backend: impl Into<String>) {
        self.active_render_backend.set(backend.into())
            .expect("Active render backend may only be set once");
    }

    pub fn get_config(&self) -> &EngineConfig {
        self.engine_config.get().expect("Engine config is not yet committed")
    }

    pub fn get_config_mut(&self) -> MutexGuard<EngineConfig> {
        if !self.is_current_thread_update_thread() {
            panic!("Engine config cannot be updated outside of update thread");
        }
        if self.engine_config.get().is_some() {
            panic!("Engine config was already committed");
        }
        self.engine_config_staging.get().lock()
    }

    pub(crate) fn commit_config(&self) {
        self.engine_config.set(self.engine_config_staging.get().lock().clone())
            .expect("Engine config was already committed");
    }

    pub fn is_current_thread_update_thread(&self) -> bool {
        let cur_id = std::thread::current().id();
        let expected_id = *self.update_thread_id.get().expect("Update thread ID is not yet set");
        cur_id == expected_id
    }

    pub(crate) fn set_update_thread_id(&self) {
        self.update_thread_id.set(std::thread::current().id())
            .expect("Update thread ID may only be set once");
    }

    pub fn add_update_callback<F: 'static + Send + Fn(Duration)>(&self, f: F, ordering: Ordering)
        -> u64 {
        let index = self.next_index();
        self.update_callbacks.insert(index, EngineCallback::new(Box::new(f), ordering));
        index
    }

    pub fn remove_update_callback(&self, index: u64) {
        self.update_callbacks.remove(index);
    }

    pub fn add_render_callback<F: 'static + Send + Fn(Duration)>(&self, f: F, ordering: Ordering)
        -> u64 {
        let index = self.next_index();
        self.render_callbacks.insert(index, EngineCallback::new(Box::new(f), ordering));
        index
    }

    pub fn remove_render_callback(&self, index: u64) {
        self.render_callbacks.remove(index);
    }

    pub fn add_one_off_update_callback<F: 'static + Send + FnOnce()>(&self, f: F) -> u64 {
        let index = self.next_index();
        self.update_one_off_callbacks.insert(index, Box::new(f,));
        index
    }

    pub fn add_one_off_render_callback<F: 'static + Send + FnOnce()>(&self, f: F) -> u64 {
        let index = self.next_index();
        self.render_one_off_callbacks.insert(index, Box::new(f));
        index
    }

    pub fn add_event_handler<T: ArgusEvent, F: 'static + Send + Fn(&T)>(
        &self,
        f: F,
        target_thread: TargetThread,
        ordering: Ordering
    )
        -> u64 {
        let callbacks_map = match target_thread {
            TargetThread::Update => {
                &self.update_event_handlers
            }
            TargetThread::Render => {
                &self.render_event_handlers
            }
        };
        let index = self.next_index();
        let wrapper = move |event_arc: &Arc<dyn ArgusEvent>| {
            let event_ref = event_arc.as_any_ref().downcast_ref::<T>().unwrap();
            f(event_ref);
        };
        callbacks_map.insert(
            index,
            (EngineCallback::new(Box::new(wrapper), ordering), TypeId::of::<T>()),
        );
        index
    }

    pub fn add_boxed_event_handler<T: ArgusEvent>(
        &self,
        f: Box<dyn 'static + Send + Fn(&T)>,
        target_thread: TargetThread,
        ordering: Ordering
    )
        -> u64 {
        let callbacks_map = match target_thread {
            TargetThread::Update => {
                &self.update_event_handlers
            }
            TargetThread::Render => {
                &self.render_event_handlers
            }
        };
        let index = self.next_index();
        let wrapper = move |event_arc: &Arc<dyn ArgusEvent>| {
            let event_ref = event_arc.as_any_ref().downcast_ref::<T>().unwrap();
            f.as_ref()(event_ref);
        };
        callbacks_map.insert(
            index,
            (EngineCallback::new(Box::new(wrapper), ordering), TypeId::of::<T>()),
        );
        index
    }

    pub fn remove_event_handler(&self, index: u64, target_thread: TargetThread) {
        let callbacks_map = match target_thread {
            TargetThread::Update => {
                &self.update_event_handlers
            }
            TargetThread::Render => {
                &self.render_event_handlers
            }
        };
        callbacks_map.remove(index);
    }

    fn next_index(&self) -> u64 {
        self.next_callback_index.fetch_add(1, atomic::Ordering::Relaxed)
    }

    pub fn push_event<T: ArgusEvent>(&self, event: T) {
        let event_arc: Arc<dyn ArgusEvent> = Arc::new(event);
        {
            let mut queue = self.update_event_queue.lock();
            queue.push((Arc::clone(&event_arc), TypeId::of::<T>()));
        }
        {
            let mut queue = self.render_event_queue.lock();
            queue.push((event_arc, TypeId::of::<T>()));
        }
    }
}
