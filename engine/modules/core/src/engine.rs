use std::convert::Infallible;
use std::{process, thread};
use std::sync::{atomic, mpsc, Condvar, Mutex};
use std::sync::atomic::AtomicBool;
use std::sync::mpsc::Receiver;
use std::time::{Duration, Instant};
use argus_logging::{debug, LogManager, LogSettings};
use itertools::Itertools;
use argus_scripting_bind::script_bind;
use argus_util::math::Vector2u;
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

static IS_STOP_REQUESTED: AtomicBool = AtomicBool::new(false);
static STOP_ACK_UPDATE: (Mutex<bool>, Condvar) = (Mutex::new(false), Condvar::new());
static STOP_ACK_RENDER: (Mutex<bool>, Condvar) = (Mutex::new(false), Condvar::new());

pub struct RenderLoopParams {
    pub core_render_callback: fn(Duration),
    pub shutdown_notifier: Receiver<fn()>,
}

#[derive(Clone, Copy, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[script_bind]
pub enum Ordering {
    First,
    Early,
    Standard,
    Late,
    Last,
}

pub fn init_logger(settings: LogSettings) {
    LogManager::initialize(settings).expect("Failed to initialize logging");
}

pub fn load_client_config(namespace: impl AsRef<str>) -> Result<(), EngineError> {
    //TODO
    set_initial_window_parameters(InitialWindowParameters {
        id: Some("default".to_owned()),
        title: Some("Dummy".to_owned()),
        mode: None,
        vsync: None,
        mouse_visible: None,
        mouse_captured: None,
        mouse_raw_input: None,
        position: None,
        dimensions: Some(Vector2u::new(600, 600)),
    });
    set_scripting_parameters(ScriptingParameters {
        main: Some(format!("{}:scripts/lua/init", namespace.as_ref())),
    });
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
        tx.send(render_shutdown_callback)
            .expect("Failed to notify render thread of engine shutdown");
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

pub fn run_on_game_thread<F: 'static + Send + FnOnce()>(callback: F) {
    EngineManager::instance().add_one_off_update_callback(callback);
}

pub fn is_current_thread_update_thread() -> bool {
    EngineManager::instance().is_current_thread_update_thread()
}

fn do_update_loop() {
    let mut last_update = Instant::now();

    while !IS_STOP_REQUESTED.load(atomic::Ordering::Relaxed) {
        let cur_instant = Instant::now();
        let delta = cur_instant.duration_since(last_update);
        last_update = cur_instant;

        for (_, callback) in EngineManager::instance().update_one_off_callbacks.drain() {
            callback();
        }

        {
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

        EngineManager::instance().update_one_off_callbacks.flush_queues();
        EngineManager::instance().update_event_handlers.flush_queues();
        EngineManager::instance().update_callbacks.flush_queues();

        for callback in EngineManager::instance().update_callbacks.items().values()
            .sorted_by_key(|callback| callback.ordering) {
            (callback.f)(delta);
        }
    }
}

fn do_render_callbacks(delta: Duration) {
    for (_, callback) in EngineManager::instance().render_one_off_callbacks.drain() {
        callback();
    }

    {
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

    EngineManager::instance().render_one_off_callbacks.flush_queues();
    EngineManager::instance().render_event_handlers.flush_queues();
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

