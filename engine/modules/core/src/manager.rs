use crate::buffered_map::BufferedMap;
use crate::{ArgusEvent, Ordering};
use crate::{EngineError, LifecycleStage, RenderLoopParams, TargetThread, LOGGER};
use argus_logging::{debug, info, warn};
use arp::ResourceIdentifier;
use fragile::Fragile;
use parking_lot::Mutex;
use serde::de::DeserializeOwned;
use std::any::{Any, TypeId};
use std::cell::RefCell;
use std::collections::HashMap;
use std::fmt::Debug;
use std::fs::File;
use std::io::{BufReader, Read};
use std::path::{Path, PathBuf};
use std::str::FromStr;
use std::sync::atomic::{AtomicU32, AtomicU64};
use std::sync::mpsc::Sender;
use std::sync::{atomic, Arc, OnceLock, RwLock, RwLockReadGuard, RwLockWriteGuard};
use std::thread::ThreadId;
use std::time::Duration;
use std::{env, fs};

const ARP_FILE_EXTENSION: &str = "arp";
const CONFIG_BASE_NAME: &str = "client";
const CONFIG_FILE_NAME: &str = "client.json";
const CONFIG_FILE_MEDIA_TYPE: &str = "application/json";
const DEF_RESOURCES_DIR_NAME: &str = "resources";

static INSTANCE: OnceLock<EngineManager> = OnceLock::new();

pub type ConfigMaybeDeserializer =
    Box<dyn Fn(&str) -> serde_json::error::Result<Box<dyn Any + Send + Sync>>>;

pub(crate) struct EngineCallback<F: ?Sized> {
    pub(crate) f: Box<F>,
    pub(crate) ordering: Ordering,
}

impl<F: ?Sized> EngineCallback<F> {
    fn new(f: Box<F>, ordering: Ordering) -> Self {
        Self { f, ordering }
    }
}

pub(crate) type CallbackOnce = Box<dyn Send + FnOnce()>;
pub(crate) type EventHandler = EngineCallback<dyn Send + Fn(&Arc<dyn ArgusEvent>)>;
pub(crate) type EventQueue = Vec<(Arc<dyn ArgusEvent>, TypeId)>;

#[derive(Default)]
pub struct EngineManager {
    cur_stage: AtomicU32,

    config: Arc<RwLock<EngineConfig>>,
    config_deserializers: Fragile<RefCell<HashMap<String, (TypeId, ConfigMaybeDeserializer)>>>,
    primary_namespace: OnceLock<String>,

    pub(crate) render_loop: OnceLock<fn(RenderLoopParams)>,
    pub(crate) render_shutdown_tx: OnceLock<Sender<fn()>>,
    pub(crate) update_thread_id: OnceLock<ThreadId>,

    next_callback_index: AtomicU64,
    pub(crate) update_callbacks: BufferedMap<u64, EngineCallback<dyn Send + Fn(Duration)>>,
    pub(crate) render_callbacks: BufferedMap<u64, EngineCallback<dyn Send + Fn(Duration)>>,
    pub(crate) update_one_off_callbacks: BufferedMap<u64, (CallbackOnce, Ordering)>,
    pub(crate) render_one_off_callbacks: BufferedMap<u64, (CallbackOnce, Ordering)>,
    pub(crate) render_init_callbacks: BufferedMap<u64, (CallbackOnce, Ordering)>,

    pub(crate) update_event_handlers: BufferedMap<u64, (EventHandler, TypeId)>,
    pub(crate) render_event_handlers: BufferedMap<u64, (EventHandler, TypeId)>,
    pub(crate) update_event_queue: Arc<Mutex<EventQueue>>,
    pub(crate) render_event_queue: Arc<Mutex<EventQueue>>,
}

pub trait EngineConfigSection: Any + Send + Sync {
}

#[derive(Debug, Default)]
pub struct EngineConfig {
    sections: HashMap<TypeId, Box<dyn Any + Send + Sync>>,
}

impl EngineConfig {
    pub fn get_section<T: 'static>(&self) -> Option<&T> {
        self.sections.get(&TypeId::of::<T>())?.as_ref().downcast_ref::<T>()
    }

    pub fn get_section_mut<T: 'static>(&mut self) -> Option<&mut T> {
        self.sections.get_mut(&TypeId::of::<T>())?.as_mut().downcast_mut::<T>()
    }
}

pub type ConfigDeserializer = fn(section: &str) -> Box<dyn Any + Send + Sync>;

impl EngineManager {
    pub fn instance() -> &'static Self {
        INSTANCE.get_or_init(Self::new)
    }

    fn new() -> Self {
        Self {
            cur_stage: AtomicU32::new(LifecycleStage::Load.into()),

            config: Arc::default(),
            config_deserializers: Default::default(),
            primary_namespace: OnceLock::default(),

            render_loop: OnceLock::new(),
            render_shutdown_tx: OnceLock::new(),
            update_thread_id: OnceLock::new(),

            next_callback_index: AtomicU64::new(1),
            update_callbacks: BufferedMap::new(),
            render_callbacks: BufferedMap::new(),
            update_one_off_callbacks: BufferedMap::new(),
            render_one_off_callbacks: BufferedMap::new(),
            render_init_callbacks: BufferedMap::new(),

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

    /// Adds a deserializer to be applied when the engine config is loaded.
    ///
    /// This should be called during [LifecycleStage::PreInit].
    pub fn add_config_deserializer<T: 'static + DeserializeOwned + Send + Sync>(
        &self,
        section_key: impl AsRef<str>,
    ) {
        self.config_deserializers.get().borrow_mut().insert(
            section_key.as_ref().to_string(),
            (
                TypeId::of::<T>(),
                Box::new(move |s| {
                    serde_json::de::from_reader::<_, T>(BufReader::new(s.as_bytes()))
                        .map(|obj| Box::new(obj) as Box<dyn Any + Send + Sync>)
                })
            )
        );
    }

    pub(crate) fn set_primary_namespace(&self, namespace: impl Into<String>) {
        self.primary_namespace.set(namespace.into())
            .expect("Cannot set primary namespace more than once");
    }

    pub(crate) fn load_config(&self, resources_path: Option<&str>)
        -> Result<(), String> {
        let mut config = self.config.write().unwrap();

        let real_resources_path = match resources_path {
            Some(path) => PathBuf::from_str(path).map_err(|err| err.to_string())?,
            None => {
                let mut wd = env::current_dir().map_err(|err| err.to_string())?;
                wd.push(DEF_RESOURCES_DIR_NAME);
                wd
            }
        };

        let namespace = self.primary_namespace.get()
            .expect("Primary namespace was not set");
        let Some(config_str) = try_load_config_contents_from_file(&real_resources_path, namespace)
            .or_else(|| try_load_config_contents_from_arp(&real_resources_path, namespace)) else {


            return Err("Unable to locate engine config!".to_owned());
        };

        let config_root: serde_json::value::Value =
            serde_json::from_str(&config_str).map_err(|e| e.to_string())?;
        let serde_json::value::Value::Object(root_map) = config_root else {
            return Err("Root value of config JSON is not an object".to_owned());
        };

        let deserializers = self.config_deserializers.get().borrow();
        for (key, val) in root_map {
            let Some((type_id, deserializer)) = deserializers.get(&key) else {
                info!(LOGGER, "Ignoring unknown key '{}' in config root", key);
                continue;
            };
            debug!(LOGGER, "Attempting to deserialize config section '{}'", key);
            let val_raw = serde_json::value::to_raw_value(&val).map_err(|e| e.to_string())?;
            let section_obj = deserializer(val_raw.get()).map_err(|e| e.to_string())?;
            debug!(LOGGER, "Successfully deserialized config section '{}'", key);

            config.sections.insert(*type_id, section_obj);
        }

        Ok(())
    }

    pub fn get_config(&self) -> RwLockReadGuard<EngineConfig> {
        self.config.read().unwrap()
    }

    pub fn get_config_mut(&self) -> RwLockWriteGuard<EngineConfig> {
        if !self.is_current_thread_update_thread() {
            panic!("Engine config cannot be updated outside of update thread");
        }
        self.config.write().unwrap()
    }

    pub fn get_render_loop(&self) -> Option<&fn(RenderLoopParams)> {
        self.render_loop.get()
    }
    
    pub fn set_render_loop(&self, f: fn(RenderLoopParams)) -> Result<(), EngineError> {
        self.render_loop.set(f)
            .map_err(|_| EngineError::new("Render loop was already registered"))
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

    pub fn add_one_off_update_callback<F: 'static + Send + FnOnce()>(
        &self, f: F,
        ordering: Ordering,
    ) -> u64 {
        let index = self.next_index();
        self.update_one_off_callbacks.insert(index, (Box::new(f), ordering));
        index
    }

    pub fn add_one_off_render_callback<F: 'static + Send + FnOnce()>(
        &self,
        f: F,
        ordering: Ordering,
    ) -> u64 {
        let index = self.next_index();
        self.render_one_off_callbacks.insert(index, (Box::new(f), ordering));
        index
    }

    pub fn add_render_init_callback<F: 'static + Send + FnOnce()>(
        &self,
        f: F,
        ordering: Ordering,
    ) -> u64 {
        let index = self.next_index();
        self.render_init_callbacks.insert(index, (Box::new(f), ordering));
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

fn try_load_config_contents_from_file(search_path: &Path, namespace: &str) -> Option<String> {
    let config_path = search_path.join(namespace).join(CONFIG_FILE_NAME);

    if !config_path.is_file() {
        return None;
    }

    let mut file = match File::open(&config_path) {
        Ok(file) => file,
        Err(err) => {
            warn!(
                LOGGER,
                "Failed to open file {} while attempting to load engine config: {}",
                config_path.display(),
                err,
            );
            return None;
        }
    };
    let mut file_contents = if let Ok(file_metadata) = file.metadata() {
        String::with_capacity(file_metadata.len() as usize)
    } else {
        String::new()
    };

    match file.read_to_string(&mut file_contents) {
        Ok(_) => {}
        Err(err) => {
            warn!(
                LOGGER,
                "Failed to read file {} while attempting to load engine config: {}",
                config_path.display(),
                err,
            );
            return None;
        }
    };

    info!(LOGGER, "Found engine config at {}", config_path.display());

    Some(file_contents)
}

fn try_load_config_contents_from_arp(search_path: &Path, namespace: &str) -> Option<String> {
    if !search_path.is_dir() {
        warn!(LOGGER, "Failed to open resources directory: {}", search_path.display());
        return None;
    }

    let mut candidate_packages: Vec<PathBuf> = Vec::new();

    let Ok(dir_iter) = fs::read_dir(search_path) else {
        warn!(LOGGER, "Failed to read resources directory: {}", search_path.display());
        return None;
    };
    for child in dir_iter {
        let child_path = match &child {
            Ok(child) => child.path(),
            Err(err) => {
                warn!(LOGGER, "Failed to read resources directory child {:?}: {}", child, err);
                return None;
            }
        };
        if child_path.extension()?.to_str().is_none_or(|ext| ext != ARP_FILE_EXTENSION) {
            continue;
        }

        if !arp::Package::is_base_archive(&child_path).unwrap_or(false) {
            continue;
        }

        let package_meta = match arp::Package::load_meta_from_file(&child_path) {
            Ok(meta) => meta,
            Err(err) => {
                warn!(
                    LOGGER,
                    "Failed to load package {} while searching for config: {}",
                    child_path.display(),
                    err,
                );
                continue;
            }
        };

        if package_meta.namespace == namespace {
            candidate_packages.push(child_path);
        }
    }

    for candidate in candidate_packages {
        let Some(file_name) = candidate.file_name() else {
            continue;
        };

        debug!(
            LOGGER,
            "Searching for client config in package {} (namespace matches)",
            file_name.to_string_lossy(),
        );

        let package = match arp::Package::load_from_file(&candidate) {
            Ok(package) => package,
            Err(err) => {
                warn!(
                    LOGGER,
                    "Failed to load package at {} while searching for config: {}",
                    file_name.to_string_lossy(),
                    err,
                );
                continue;
            }
        };

        let res_meta = match package.find_resource(
            &ResourceIdentifier::new(namespace, [CONFIG_BASE_NAME.to_owned()])
        ) {
            Ok(meta) => meta,
            Err(err) => {
                debug!(
                    LOGGER,
                    "Did not find config in package {}: {}",
                    file_name.to_string_lossy(),
                    err,
                );
                continue;
            }
        };

        if res_meta.media_type != CONFIG_FILE_MEDIA_TYPE {
            warn!(
                LOGGER,
                "File '{}' in package {} has unexpected media type {}, \
                cannot load as client config",
                CONFIG_BASE_NAME,
                file_name.to_string_lossy(),
                res_meta.media_type,
            );
            continue;
        }

        let res_contents = match res_meta.load() {
            Ok(contents) => contents,
            Err(err) => {
                warn!(
                    LOGGER,
                    "Failed to load engine config from package {}: {}",
                    file_name.to_string_lossy(),
                    err,
                );
                continue;
            }
        };

        let res_contents_str = match String::from_utf8(res_contents) {
            Ok(contents) => contents,
            Err(err) => {
                warn!(LOGGER, "Engine config in package {} was not valid UTF-8", err);
                continue;
            }
        };

        info!(LOGGER, "Found engine config in package at {}", file_name.to_string_lossy());

        return Some(res_contents_str);
    }

    None
}
