mod callback;
mod config;

use crate::*;
use crate::engine::config::*;
use crate::engine::callback::CallbackList;
use crate::event::*;
use crate::module::{DynamicModule, StaticModule};

use std::collections::HashMap;
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

#[derive(Default)]
struct ModuleState {
    dyn_module_registrations: HashMap<String, DynamicModule>,
    enabled_static_modules: Vec<StaticModule>,
    enabled_dyn_modules_staging: HashMap<String, DynamicModule>,
    enabled_dyn_modules: Vec<DynamicModule>,
}

#[derive(Clone)]
struct RunningState {
    is_stopping: Arc<Mutex<bool>>,
    game_thread_acknowledged_halt: Arc<Mutex<bool>>,
    force_shutdown_on_next_interrupt: Arc<Mutex<bool>>,
    render_thread_halted: Arc<(Mutex<bool>, Condvar)>,
}

impl Default for RunningState {
    fn default() -> Self {
        RunningState {
            is_stopping: Arc::new(Mutex::new(false)),
            game_thread_acknowledged_halt: Arc::new(Mutex::new(false)),
            force_shutdown_on_next_interrupt: Arc::new(Mutex::new(false)),
            render_thread_halted: Arc::new((Mutex::new(true), Condvar::new())),
        }
    }
}

struct EngineCallbacks {
    update_callbacks: Arc<RwLock<CallbackList<DeltaCallback>>>,
    render_callbacks: Arc<RwLock<CallbackList<DeltaCallback>>>,
    one_off_callbacks: Mutex<Vec<NullaryCallback>>,
}

impl Default for EngineCallbacks {
    fn default() -> Self {
        EngineCallbacks {
            update_callbacks: Arc::new(RwLock::new(CallbackList::new())),
            render_callbacks: Arc::new(RwLock::new(CallbackList::new())),
            one_off_callbacks: Mutex::new(Vec::new()),
        }
    }
}

pub struct EngineHandle {
    client_info: ClientInfo,
    engine_config: EngineConfig,

    module_state: ModuleState,
    running_state: RunningState,
    callbacks: EngineCallbacks,
}

pub fn configure_engine(config_namespace: &str) -> Box<EngineHandle> {
    configure_engine_explicitly(ClientInfo::default(), EngineConfig::default())
}

pub fn configure_engine_explicitly(client_info: ClientInfo, engine_config: EngineConfig) -> Box<EngineHandle> {
    Box::new(EngineHandle {
        client_info,
        engine_config,
        module_state: Default::default(),
        running_state: Default::default(),
        callbacks: Default::default(),
    })
}

impl EngineHandle {
    pub fn initialize(&mut self) {
        logger.info("Engine initialization started");

        logger.debug("Enabling requested modules");

        if !self.engine_config.load_modules.is_empty() {
            self.enable_modules(self.engine_config.load_modules.clone());
        } else {
            self.enable_modules(vec![MODULE_CORE.to_string()]);
        }

        //load_dynamic_modules();

        logger.debug("Initializing enabled modules");

        self.init_modules();

        logger.info("Engine initialized!");
    }

    pub fn start(&mut self, game_loop: DeltaCallback) {
        logger.info("Bringing up engine");

        logger.fatal_if(*g_trunk_initialized.read().unwrap(), "Cannot start engine before it is initialized.");

        /*logger.fatal_if(!get_client_id().empty(), "Client ID must be set prior to engine start");
        logger.fatal_if(!get_client_name().empty(), "Client ID must be set prior to engine start");
        logger.fatal_if(!get_client_version().empty(), "Client ID must be set prior to engine start");*/

        self.register_update_callback(game_loop);

        let target_fps = self.engine_config.target_framerate;
        let running_state_copy = self.running_state.clone();
        let render_callbacks = self.callbacks.render_callbacks.clone();

        let g_render_thread_jh = std::thread::spawn(move || render_loop(target_fps,
            running_state_copy, render_callbacks));

        //ctrlc::set_handler(|| self.stop_engine());

        logger.info("Engine started! Passing control to game loop.");

        // pass control over to the game loop
        self.game_loop();

        logger.info("Game loop has halted, exiting program");

        exit(0);
    }

    pub fn stop_engine(&mut self) {
        if *self.running_state.force_shutdown_on_next_interrupt.lock().unwrap() {
            logger.info("Forcibly terminating process");
            exit(0);
        } else if *self.running_state.game_thread_acknowledged_halt.lock().unwrap() {
            logger.info("Forcibly proceeding with engine bring-down");
            *self.running_state.force_shutdown_on_next_interrupt.lock().unwrap() = true;

            // bit of a hack to trick the game thread into thinking the render thread halted
            let (lock, cvar) = &*self.running_state.render_thread_halted;
            let mut halted = lock.lock().unwrap();
            *halted = true;
            cvar.notify_one();
        } else if *self.running_state.is_stopping.lock().unwrap() {
            logger.warn("Engine is already halting");
        }

        logger.info("Engine halt requested");

        logger.fatal_if(*g_trunk_initialized.read().unwrap(), "Cannot stop engine before it is initialized.");

        *self.running_state.is_stopping.lock().unwrap() = true;
    }

    pub fn register_update_callback(&mut self, callback: DeltaCallback) -> Index {
        logger.fatal_if(*g_trunk_initializing.read().unwrap() || *g_trunk_initialized.read().unwrap(),
                "Cannot register update callback before engine initialization.");
        return self.callbacks.update_callbacks.write().unwrap().add(callback);
    }

    pub fn unregister_update_callback(&mut self, id: Index) {
        self.callbacks.update_callbacks.write().unwrap().remove(id);
    }

    pub fn register_render_callback(&mut self, callback: DeltaCallback) -> Index {
        logger.fatal_if(*g_trunk_initializing.read().unwrap() || *g_trunk_initialized.read().unwrap(),
                "Cannot register render callback before engine initialization.");
        return self.callbacks.render_callbacks.write().unwrap().add(callback);
    }

    pub fn unregister_render_callback(&mut self, id: Index) {
        (&*self.callbacks.render_callbacks).write().unwrap().remove(id);
    }

    pub fn run_on_game_thread(&mut self, callback: NullaryCallback) {
        self.callbacks.one_off_callbacks.lock().unwrap().push(callback);
    }

    fn deinit_callbacks(&mut self) {
        self.callbacks.update_callbacks.write().unwrap().clear();
        self.callbacks.render_callbacks.write().unwrap().clear();
    }

    pub(crate) fn kill_game_thread(&mut self, ) {
        /*g_game_thread_join_handle->detach();
        g_game_thread_join_handle->destroy();*/
        //TODO
    }

    fn game_loop(&mut self) {
        let mut last_update: Option<Instant> = None;

        loop {
            if *self.running_state.is_stopping.lock().unwrap() {
                logger.debug("Engine halt request is acknowledged game thread");
                *self.running_state.game_thread_acknowledged_halt.lock().unwrap() = true;

                // wait for render thread to finish up what it's doing so we don't interrupt it ~~and cause a segfault~~
                // edit: lol rust ftw
                {
                    let (lock, cvar) = &*self.running_state.render_thread_halted;
                    if !(*lock.lock().unwrap()) {
                        logger.debug("Game thread observed render thread was not halted, waiting on monitor (send SIGINT again to force halt)");
                        cvar.wait(lock.lock().unwrap());
                    }
                }

                // at this point all event and callback execution should have
                // stopped which allows us to start doing non-thread-safe things

                logger.debug("Game thread observed render thread is halted, proceeding with engine bring-down");

                logger.debug("Deinitializing engine modules");

                self.deinit_modules();

                logger.debug("Deinitializing event callbacks");

                // if we don't do this explicitly, the callback lists (and thus
                // the callback function objects) will be deinitialized
                // statically and will segfault on handlers registered by
                // external libraries (which will have already been unloaded)
                deinit_event_handlers();

                logger.debug("Deinitializing general callbacks");

                // same deal here
                self.deinit_callbacks();

                logger.debug("Unloading dynamic engine modules");

                self.unload_dynamic_modules();

                logger.info("Engine bring-down completed");

                break;
            }

            let update_start = Instant::now();
            let delta = compute_delta(&mut last_update);
            
            // prioritize one-off callbacks
            {
                let mut one_off_callbacks = self.callbacks.one_off_callbacks.lock().unwrap();
                for callback in &*one_off_callbacks {
                    callback();
                }
                (*one_off_callbacks).clear();
            }

            //TODO: do we need to flush the queues before the engine stops?
            self.callbacks.update_callbacks.write().unwrap().flush();
            flush_event_listener_queues(TargetThread::Update);

            // invoke update callbacks
            for callback in &self.callbacks.update_callbacks.read().unwrap().list {
                (callback.value)(delta);
            }

            process_event_queue(TargetThread::Update);

            if let Some(tickrate) = self.engine_config.target_tickrate {
                handle_idle(update_start, tickrate);
            }
        }
    }
}

fn render_loop(target_framerate: Option<u32>, running_state: RunningState,
    render_callbacks: Arc<RwLock<CallbackList<DeltaCallback>>>) {
    let mut last_frame: Option<Instant> = None;

    loop {
        if *running_state.is_stopping.lock().unwrap() {
            logger.debug("Engine halt request is acknowledged by render thread");
            let (lock, cvar) = &*running_state.render_thread_halted;
            let mut halted = lock.lock().unwrap();
            *halted = true;
            cvar.notify_one();
            break;
        }

        let render_start = Instant::now();
        let delta = compute_delta(&mut last_frame);

        render_callbacks.write().unwrap().flush();
        flush_event_listener_queues(TargetThread::Render);

        // invoke render callbacks
        for callback in &render_callbacks.read().unwrap().list {
            (callback.value)(delta);
        }

        process_event_queue(TargetThread::Render);

        if let Some(framerate) = target_framerate {
            handle_idle(render_start, framerate);
        }
    }
}

fn handle_idle(start_timestamp: Instant, target_rate: u32) {
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

fn compute_delta(last_timestamp: &mut Option<Instant>) -> Duration {
    let delta = match last_timestamp {
        Some(ts) => Instant::now().duration_since(*ts),
        None => Duration::new(0, 0)
    };

    *last_timestamp = Some(Instant::now());

    return delta;
}
