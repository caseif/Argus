mod callback;
mod config;

use crate::*;
use crate::engine::config::*;
use crate::engine::callback::CallbackList;
use crate::event::*;
use crate::module::*;

use lazy_static::lazy_static;

use std::process::exit;
use std::sync::RwLock;
use std::sync::{Arc, Condvar, Mutex};
use std::thread::sleep;
use std::time::{Duration, Instant};

type Index = u64;
type NullaryCallback = fn();
type DeltaCallback = fn(Duration);

const NS_PER_US: u64 = 1_000;
const US_PER_S: u64 = 1_000_000;
const SLEEP_OVERHEAD_NS: Duration = Duration::from_nanos(120_000);

struct EngineState {
    is_stopping: Mutex<bool>,
    game_thread_acknowledged_halt: Mutex<bool>,
    force_shutdown_on_next_interrupt: Mutex<bool>,
    render_thread_halted: Arc<(Mutex<bool>, Condvar)>,

    update_callbacks: Arc<RwLock<CallbackList<DeltaCallback>>>,
    render_callbacks: Arc<RwLock<CallbackList<DeltaCallback>>>,
    one_off_callbacks: Mutex<Vec<NullaryCallback>>,
}

lazy_static! {
    static ref g_engine_state: EngineState = EngineState {
        is_stopping: Mutex::new(false),
        game_thread_acknowledged_halt: Mutex::new(false),
        force_shutdown_on_next_interrupt: Mutex::new(false),
        render_thread_halted: Arc::new((Mutex::new(true), Condvar::new())),

        update_callbacks: Arc::new(RwLock::new(CallbackList::new())),
        render_callbacks: Arc::new(RwLock::new(CallbackList::new())),
        one_off_callbacks: Mutex::new(Vec::new()),
    };
    
    static ref g_engine_config: EngineConfig = Default::default();
}

pub fn initialize_engine() {
    logger.info("Engine initialization started");

    logger.debug("Enabling requested modules");

    if !g_engine_config.load_modules.is_empty() {
        enable_modules(g_engine_config.load_modules.clone());
    } else {
        enable_modules(vec![MODULE_CORE.to_string()]);
    }

    //load_dynamic_modules();

    logger.debug("Initializing enabled modules");

    init_modules();

    logger.info("Engine initialized!");
}

pub fn start_engine(game_loop: DeltaCallback) {
    logger.info("Bringing up engine");

    logger.fatal_if(*g_trunk_initialized.read().unwrap(), "Cannot start engine before it is initialized.");

    /*logger.fatal_if(!get_client_id().empty(), "Client ID must be set prior to engine start");
    logger.fatal_if(!get_client_name().empty(), "Client ID must be set prior to engine start");
    logger.fatal_if(!get_client_version().empty(), "Client ID must be set prior to engine start");*/

    register_update_callback(game_loop);

    let g_game_thread_jh = std::thread::spawn(_render_loop);

    ctrlc::set_handler(|| stop_engine());

    logger.info("Engine started! Passing control to game loop.");

    // pass control over to the game loop
    _game_loop();

    logger.info("Game loop has halted, exiting program");

    exit(0);
}

pub fn stop_engine() {
    if *g_engine_state.force_shutdown_on_next_interrupt.lock().unwrap() {
        logger.info("Forcibly terminating process");
        exit(0);
    } else if *g_engine_state.game_thread_acknowledged_halt.lock().unwrap() {
        logger.info("Forcibly proceeding with engine bring-down");
        *g_engine_state.force_shutdown_on_next_interrupt.lock().unwrap() = true;

        // bit of a hack to trick the game thread into thinking the render thread halted
        let (lock, cvar) = &*g_engine_state.render_thread_halted;
        let mut halted = lock.lock().unwrap();
        *halted = true;
        cvar.notify_one();
    } else if *g_engine_state.is_stopping.lock().unwrap() {
        logger.warn("Engine is already halting");
    }

    logger.info("Engine halt requested");

    logger.fatal_if(*g_trunk_initialized.read().unwrap(), "Cannot stop engine before it is initialized.");

    *g_engine_state.is_stopping.lock().unwrap() = true;
}

pub fn register_update_callback(callback: DeltaCallback) -> Index {
    logger.fatal_if(*g_trunk_initializing.read().unwrap() || *g_trunk_initialized.read().unwrap(),
            "Cannot register update callback before engine initialization.");
    return g_engine_state.update_callbacks.write().unwrap().add(callback);
}

pub fn unregister_update_callback(id: Index) {
    g_engine_state.update_callbacks.write().unwrap().remove(id);
}

pub fn register_render_callback(callback: DeltaCallback) -> Index {
    logger.fatal_if(*g_trunk_initializing.read().unwrap() || *g_trunk_initialized.read().unwrap(),
            "Cannot register render callback before engine initialization.");
    return g_engine_state.render_callbacks.write().unwrap().add(callback);
}

pub fn unregister_render_callback(id: Index) {
    (&*g_engine_state.render_callbacks).write().unwrap().remove(id);
}

fn _deinit_callbacks() {
    g_engine_state.update_callbacks.write().unwrap().clear();
    g_engine_state.render_callbacks.write().unwrap().clear();
}

pub fn run_on_game_thread(callback: NullaryCallback) {
    g_engine_state.one_off_callbacks.lock().unwrap().push(callback);
}

pub(crate) fn kill_game_thread() {
    /*g_game_thread_join_handle->detach();
    g_game_thread_join_handle->destroy();*/
    //TODO
}

fn _handle_idle(start_timestamp: Instant, target_rate: u32) {
    if target_rate == 0 {
        return;
    }

    let delta = Instant::now() - start_timestamp;

    let frametime_target = Duration::from_micros(US_PER_S / target_rate as u64);
    if delta < frametime_target {
        let sleep_time_ns = frametime_target - delta;
        if sleep_time_ns <= SLEEP_OVERHEAD_NS {
            return;
        }
        sleep(sleep_time_ns - SLEEP_OVERHEAD_NS);
    }
}

fn _compute_delta(last_timestamp: &mut Option<Instant>) -> Duration {
    let delta = match last_timestamp {
        Some(ts) => Instant::now().duration_since(*ts),
        None => Duration::new(0, 0)
    };

    *last_timestamp = Some(Instant::now());

    return delta;
}

fn _game_loop() {
    let mut last_update: Option<Instant> = None;

    loop {
        if *g_engine_state.is_stopping.lock().unwrap() {
            logger.debug("Engine halt request is acknowledged game thread");
            *g_engine_state.game_thread_acknowledged_halt.lock().unwrap() = true;

            // wait for render thread to finish up what it's doing so we don't interrupt it ~~and cause a segfault~~
            // edit: lol rust ftw
            {
                let (lock, cvar) = &*g_engine_state.render_thread_halted;
                if !(*lock.lock().unwrap()) {
                    logger.debug("Game thread observed render thread was not halted, waiting on monitor (send SIGINT again to force halt)");
                    cvar.wait(lock.lock().unwrap());
                }
            }

            // at this point all event and callback execution should have
            // stopped which allows us to start doing non-thread-safe things

            logger.debug("Game thread observed render thread is halted, proceeding with engine bring-down");

            logger.debug("Deinitializing engine modules");

            deinit_modules();

            logger.debug("Deinitializing event callbacks");

            // if we don't do this explicitly, the callback lists (and thus
            // the callback function objects) will be deinitialized
            // statically and will segfault on handlers registered by
            // external libraries (which will have already been unloaded)
            deinit_event_handlers();

            logger.debug("Deinitializing general callbacks");

            // same deal here
            _deinit_callbacks();

            logger.debug("Unloading dynamic engine modules");

            unload_dynamic_modules();

            logger.info("Engine bring-down completed");

            break;
        }

        let update_start = Instant::now();
        let delta = _compute_delta(&mut last_update);
        
        // prioritize one-off callbacks
        {
            let mut one_off_callbacks = g_engine_state.one_off_callbacks.lock().unwrap();
            for callback in &*one_off_callbacks {
                callback();
            }
            (*one_off_callbacks).clear();
        }

        //TODO: do we need to flush the queues before the engine stops?
        g_engine_state.update_callbacks.write().unwrap().flush();
        flush_event_listener_queues(TargetThread::Update);

        // invoke update callbacks
        for callback in &g_engine_state.update_callbacks.read().unwrap().list {
            (callback.value)(delta);
        }

        process_event_queue(TargetThread::Update);

        if let Some(tickrate) = g_engine_config.target_tickrate {
            _handle_idle(update_start, tickrate);
        }
    }
}

fn _render_loop() {
    let mut last_frame: Option<Instant> = None;

    loop {
        if *g_engine_state.is_stopping.lock().unwrap() {
            logger.debug("Engine halt request is acknowledged by render thread");
            let (lock, cvar) = &*g_engine_state.render_thread_halted;
            let mut halted = lock.lock().unwrap();
            *halted = true;
            cvar.notify_one();
            break;
        }

        let render_start = Instant::now();
        let delta = _compute_delta(&mut last_frame);

        g_engine_state.render_callbacks.write().unwrap().flush();
        flush_event_listener_queues(TargetThread::Render);

        // invoke render callbacks
        for callback in &g_engine_state.render_callbacks.read().unwrap().list {
            (callback.value)(delta);
        }

        process_event_queue(TargetThread::Render);

        if let Some(framerate) = g_engine_config.target_framerate {
            _handle_idle(render_start, framerate);
        }
    }
}
