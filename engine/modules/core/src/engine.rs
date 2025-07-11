use std::convert::Infallible;
use std::{process, thread};
use std::sync::{atomic, mpsc, Condvar, Mutex};
use std::sync::atomic::AtomicBool;
use std::sync::mpsc::Receiver;
use std::time::{Duration, Instant};
use itertools::Itertools;
use serde::Deserialize;
use argus_logging::{debug, severe, warn, LogManager, LogSettings};
use argus_scripting_bind::script_bind;
use crate::*;
use crate::manager::{EngineManager};
use crate::module::get_registered_modules_sorted;

const INIT_STAGES: &[LifecycleStage] = &[
    LifecycleStage::Load,
    LifecycleStage::PreInit,
    LifecycleStage::Init,
    LifecycleStage::PostInit
];

const DEINIT_STAGES: &[LifecycleStage] = &[
    LifecycleStage::PreDeinit,
    LifecycleStage::Deinit,
    LifecycleStage::PostDeinit
];

static IS_RENDER_INITTED: (Mutex<bool>, Condvar) = (Mutex::new(false), Condvar::new());
static IS_STOP_REQUESTED: AtomicBool = AtomicBool::new(false);
static STOP_ACK_UPDATE: (Mutex<bool>, Condvar) = (Mutex::new(false), Condvar::new());
static STOP_ACK_RENDER: (Mutex<bool>, Condvar) = (Mutex::new(false), Condvar::new());
static DID_RENDER_PANIC: AtomicBool = AtomicBool::new(false);

#[derive(Clone, Debug, Default, Deserialize)]
pub struct ClientConfig {
    pub id: String,
    pub name: String,
    pub version: String,
}

#[derive(Clone, Debug, Default, Deserialize)]
pub struct CoreConfig {
    pub modules: Vec<String>,
    #[serde(default)]
    pub render_backends: Vec<String>,
    #[serde(default)]
    pub screen_scale_mode: ScreenSpaceScaleMode,
    #[serde(default)]
    pub target_tickrate: Option<u32>,
    #[serde(default)]
    pub target_framerate: Option<u32>,
}

pub struct RenderLoopParams {
    pub(crate) core_render_callback: fn(&RenderLoopParams, Duration),
    pub(crate) shutdown_notifier: Receiver<fn()>,
}

impl RenderLoopParams {
    pub fn should_shutdown(&self) -> bool {
        if let Ok(shutdown_callback) = self.shutdown_notifier.try_recv() {
            shutdown_callback();
            return true;
        }

        false
    }

    pub fn run_core_callbacks(&self, delta: Duration) {
        (self.core_render_callback)(self, delta);
    }
}

#[derive(Clone, Copy, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[script_bind]
pub enum Ordering {
    Earliest,
    Early,
    Standard,
    Late,
    Latest,
}

struct PoisonPill {
    callback: fn(),
}

impl PoisonPill {
    fn new(callback: fn()) -> Self {
        Self { callback }
    }
}

impl Drop for PoisonPill {
    fn drop(&mut self) {
        if thread::panicking() {
            eprintln!("Thread panic detected!");
            (self.callback)();
        }
    }
}

pub fn init_logger(settings: LogSettings) {
    LogManager::initialize(settings).expect("Failed to initialize logging");
}

pub fn load_client_config(namespace: impl AsRef<str>) -> Result<(), EngineError> {
    EngineManager::instance().set_primary_namespace(namespace.as_ref());
    Ok(())
}

pub fn initialize_engine() -> Result<(), EngineError> {
    EngineManager::instance().set_update_thread_id();

    let registered_modules = get_registered_modules_sorted()?;

    for stage in INIT_STAGES {
        EngineManager::instance().set_current_lifecycle_stage(*stage);
        for reg in &registered_modules {
            debug!(LOGGER, "Sending lifecycle stage '{:?}' to module {}", stage, reg.id);
            (reg.entry_point)(*stage);
            debug!(LOGGER, "Module {} completed lifecycle stage '{:?}'", reg.id, stage);
        }
    }
    EngineManager::instance().set_current_lifecycle_stage(LifecycleStage::Running);

    //TODO
    Ok(())
}

pub fn set_render_loop(entry_point: fn(RenderLoopParams)) -> Result<(), EngineError> {
    let mgr = EngineManager::instance();
    mgr.set_render_loop(entry_point)
}

pub fn start_engine() -> Result<Infallible, EngineError> {
    let render_thread = {
        let mgr = EngineManager::instance();

        let (render_shutdown_tx, render_shutdown_rx) = mpsc::channel();
        mgr.render_shutdown_tx.set(render_shutdown_tx)
            .expect("Failed to store shutdown notifier for render thread");

        let render_loop = mgr.get_render_loop().expect("Render loop is not set");

        thread::spawn(|| {
            let _poison_pill =
                PoisonPill::new(|| DID_RENDER_PANIC.store(true, atomic::Ordering::Relaxed));

            debug!(LOGGER, "Running render initialization callbacks");

            mgr.render_init_callbacks.flush_queues();
            let mut callbacks = mgr.render_init_callbacks.drain();
            callbacks.sort_by_key(|(_, (_, ordering))| *ordering);
            for (_, (callback, _)) in callbacks {
                callback();
            }

            debug!(LOGGER, "Render thread is fully initialized");
            
            *IS_RENDER_INITTED.0.lock().unwrap() = true;
            IS_RENDER_INITTED.1.notify_all();
            debug!(LOGGER, "Render thread notified update thread that it can proceed");

            render_loop(RenderLoopParams {
                core_render_callback: do_render_callbacks,
                shutdown_notifier: render_shutdown_rx,
            });
        })
    };

    do_update_loop();

    debug!(LOGGER, "Update thread observed halt request");

    *STOP_ACK_UPDATE.0.lock().unwrap() = true;
    STOP_ACK_UPDATE.1.notify_all();

    debug!(LOGGER, "Update thread acknowledged halt request");

    let (render_ack_lock, render_ack_cv) = &STOP_ACK_RENDER;
    let mut render_did_ack = render_ack_lock.lock().unwrap();
    if !*render_did_ack {
        loop {
            let result = render_ack_cv
                .wait_timeout(render_did_ack, Duration::from_millis(10)).unwrap();
            render_did_ack = result.0;
            if *render_did_ack {
                debug!(
                    LOGGER,
                    "Render thread has acknowledged halt request, update thread can proceed",
                );
                break;
            }
        }
    } else {
        debug!(
            LOGGER,
            "Render thread already acknowledged halt request, update thread can proceed",
        );
    }

    let registered_modules = get_registered_modules_sorted()?;

    for stage in DEINIT_STAGES {
        EngineManager::instance().set_current_lifecycle_stage(*stage);
        for reg in registered_modules.iter().rev() {
            debug!(LOGGER, "Sending lifecycle stage '{:?}' to module {}", stage, reg.id);
            (reg.entry_point)(*stage);
            debug!(LOGGER, "Module {} completed lifecycle stage '{:?}'", reg.id, stage);
        }
    }

    if !render_thread.is_finished() {
        debug!(LOGGER, "Waiting for render thread to halt...");
        render_thread.join().unwrap();
    }

    debug!(LOGGER, "Engine stopped");

    LogManager::instance().join().unwrap();

    process::exit(0);
}

pub fn stop_engine() {
    debug!(LOGGER, "Engine stop requested");
    IS_STOP_REQUESTED.store(true, atomic::Ordering::Relaxed);
    if let Some(tx) = EngineManager::instance().render_shutdown_tx.get() {
        if tx.send(render_shutdown_callback).is_err() {
            warn!(
                LOGGER,
                "Failed to notify render thread of engine shutdown - it most likely panicked",
            );
            // pretend the render thread acknowledged the shutdown since it
            // probably isn't running anymore
            *STOP_ACK_RENDER.0.lock().unwrap() = true;
            STOP_ACK_RENDER.1.notify_all();
        };
    }
}

pub fn get_current_lifecycle_stage() -> LifecycleStage {
    EngineManager::instance().get_current_lifecycle_stage()
}

pub fn register_update_callback<F: 'static + Send + Fn(Duration)>(
    update_callback: F,
    ordering: Ordering,
) -> u64 {
    EngineManager::instance().add_update_callback(update_callback, ordering)
}

#[script_bind(rename = "register_update_callback")]
pub fn register_update_callback_bindable(update_callback: Box<dyn Fn(u64) + Send>) -> u64 {
    register_update_callback(
        move |dur: Duration| update_callback(dur.as_micros() as u64),
        Ordering::Standard
    )
}

pub fn unregister_update_callback(id: u64) {
    EngineManager::instance().remove_update_callback(id);
}

pub fn register_render_callback<F: 'static + Send + Fn(Duration)>(
    render_callback: F,
    ordering: Ordering,
) -> u64 {
    EngineManager::instance().add_render_callback(render_callback, ordering)
}

pub fn unregister_render_callback(id: u64) {
    EngineManager::instance().remove_update_callback(id);
}

pub fn run_on_update_thread<F: 'static + Send + FnOnce()>(callback: F, ordering: Ordering) {
    EngineManager::instance().add_one_off_update_callback(callback, ordering);
}

pub fn run_on_render_thread<F: 'static + Send + FnOnce()>(callback: F, ordering: Ordering) {
    EngineManager::instance().add_one_off_render_callback(callback, ordering);
}

pub fn is_current_thread_update_thread() -> bool {
    EngineManager::instance().is_current_thread_update_thread()
}

fn do_update_loop() {
    let (update_ack_lock, update_ack_cv) = &IS_RENDER_INITTED;
    let mut should_start = update_ack_lock.lock().unwrap();
    if !*should_start {
        loop {
            let result = update_ack_cv
                .wait_timeout(should_start, Duration::from_millis(10)).unwrap();
            should_start = result.0;
            if *should_start {
                debug!(
                    LOGGER,
                    "Render thread is now fully initialized, update loop can start",
                );
                break;
            }
        }
    } else {
        debug!(
            LOGGER,
            "Render thread was already initialized - update loop can start immediately",
        );
    }

    let mut last_update = Instant::now();

    while !IS_STOP_REQUESTED.load(atomic::Ordering::Relaxed) {
        let cur_instant = Instant::now();
        let delta = cur_instant.duration_since(last_update);
        last_update = cur_instant;

        if DID_RENDER_PANIC.load(atomic::Ordering::Relaxed) {
            severe!(LOGGER, "Render thread indicated panic");
            stop_engine();
            continue;
        }

        EngineManager::instance().update_one_off_callbacks.flush_queues();

        let mut once_callbacks = EngineManager::instance().update_one_off_callbacks.drain();
        once_callbacks.sort_by_key(|(_, (_, ordering))| *ordering);
        for (_, (callback, _)) in once_callbacks {
            callback();
        }

        {
            EngineManager::instance().update_event_handlers.flush_queues();

            let event_handlers = EngineManager::instance().update_event_handlers.items();
            let queued_events = EngineManager::instance().update_event_queue.lock()
                .drain(..)
                .collect::<Vec<_>>();
            for (event, event_type) in queued_events {
                for (handler, handler_type) in event_handlers.values() {
                    if *handler_type == event_type {
                        (*handler.f)(&event);
                    }
                }
            }
        }

        EngineManager::instance().update_callbacks.flush_queues();

        for callback in EngineManager::instance().update_callbacks.items().values()
            .sorted_by_key(|callback| callback.ordering) {
            (callback.f)(delta);
        }

        let config = EngineManager::instance().get_config();
        if let Some(tickrate) = config.get_section::<CoreConfig>().unwrap().target_tickrate {
            if tickrate == 0 {
                continue;
            }
            let target_tick_dur = Duration::from_micros((1000000.0 / tickrate as f32) as u64);
            let cur_tick_dur = Instant::now() - last_update + Duration::from_micros(60);
            if cur_tick_dur >= target_tick_dur {
                continue;
            }
            let sleep_dur = target_tick_dur - cur_tick_dur;
            thread::sleep(sleep_dur);
        }
    }
}

fn do_render_callbacks(_params: &RenderLoopParams, delta: Duration) {
    EngineManager::instance().render_one_off_callbacks.flush_queues();

    let mut once_callbacks = EngineManager::instance().render_one_off_callbacks.drain();
    once_callbacks.sort_by_key(|(_, (_, ordering))| *ordering);
    for (_, (callback, _)) in once_callbacks {
        callback();
    }

    {
        EngineManager::instance().render_event_handlers.flush_queues();

        let event_handlers = EngineManager::instance().render_event_handlers.items();
        let queued_events = EngineManager::instance().render_event_queue.lock()
            .drain(..)
            .collect::<Vec<_>>();
        for (event, event_type) in queued_events {
            for (handler, handler_type) in event_handlers.values() {
                if *handler_type == event_type {
                    (*handler.f)(&event);
                }
            }
        }
    }

    EngineManager::instance().render_callbacks.flush_queues();

    for callback in EngineManager::instance().render_callbacks.items().values()
        .sorted_by_key(|callback| callback.ordering) {
        (callback.f)(delta);
    }
}

fn render_shutdown_callback() {
    debug!(LOGGER, "Render thread observed halt request");

    *STOP_ACK_RENDER.0.lock().unwrap() = true;
    STOP_ACK_RENDER.1.notify_all();

    debug!(LOGGER, "Render thread acknowledged halt request");

    let (update_ack_lock, update_ack_cv) = &STOP_ACK_UPDATE;
    let mut update_did_ack = update_ack_lock.lock().unwrap();
    if !*update_did_ack {
        loop {
            let result = update_ack_cv
                .wait_timeout(update_did_ack, Duration::from_millis(10)).unwrap();
            update_did_ack = result.0;
            if *update_did_ack {
                debug!(
                    LOGGER,
                    "Update thread has acknowledged halt request, render thread can proceed",
                );
                break;
            }
        }
    } else {
        debug!(
            LOGGER,
            "Update thread already acknowledged halt request, render thread can proceed",
        );
    }
}

