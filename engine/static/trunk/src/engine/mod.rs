pub(crate) mod callback;
pub mod config;

use crate::*;
use crate::engine::config::*;
use crate::engine::callback::CallbackList;
use crate::event::*;
use crate::module::{DynamicModule, StaticModule};

use std::collections::{HashMap, VecDeque};
use std::process::exit;
use std::sync::{Arc, Condvar, Mutex, RwLock};
use std::thread::sleep;
use std::time::{Duration, Instant};

pub type Index = u64;
pub type NullaryCallback = fn();
pub type DeltaCallback = fn(Duration);

const NS_PER_US: u64 = 1_000;
const US_PER_S: u64 = 1_000_000;
const SLEEP_OVERHEAD_NS: Duration = Duration::from_nanos(120_000);

#[derive(Default)]
pub(crate) struct ModuleState {
    pub(crate) dyn_module_registrations: HashMap<String, DynamicModule>,
    pub(crate) enabled_static_modules: Vec<StaticModule>,
    pub(crate) enabled_dyn_modules_staging: HashMap<String, DynamicModule>,
    pub(crate) enabled_dyn_modules: Vec<DynamicModule>,
    pub(crate) dyn_library_handles: HashMap<String, libloading::Library>
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
            one_off_callbacks: Mutex::new(Vec::new())
        }
    }
}

pub(crate) struct EventState {
    pub(crate) update_event_listeners: Arc<RwLock<CallbackList<ArgusEventHandler>>>,
    pub(crate) render_event_listeners: Arc<RwLock<CallbackList<ArgusEventHandler>>>,
    pub(crate) update_event_queue: Arc<Mutex<VecDeque<Arc<dyn ArgusEvent + Send + Sync>>>>,
    pub(crate) render_event_queue: Arc<Mutex<VecDeque<Arc<dyn ArgusEvent + Send + Sync>>>>
}

impl Default for EventState {
    fn default() -> Self {
        EventState {
            update_event_listeners: Arc::new(RwLock::new(CallbackList::new())),
            render_event_listeners: Arc::new(RwLock::new(CallbackList::new())),
            update_event_queue: Arc::new(Mutex::new(VecDeque::new())),
            render_event_queue: Arc::new(Mutex::new(VecDeque::new()))
        }
    }
}

pub struct EngineHandle {
    client_info: ClientInfo,
    engine_config: EngineConfig,

    pub(crate) prev_stage: Option<LifecycleStage>,
    pub(crate) module_state: ModuleState,
    pub(crate) event_state: EventState,
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
        prev_stage: None,
        module_state: Default::default(),
        event_state: Default::default(),
        running_state: Default::default(),
        callbacks: Default::default(),
    })
}

impl EngineHandle {
    pub fn initialize(&mut self) {
        LOGGER.info("Engine initialization started");

        LOGGER.fatal_if(self.prev_stage.is_some(), "Cannot initialize engine more than once.");

        LOGGER.debug("Enabling requested modules");

        if !self.engine_config.load_modules.is_empty() {
            self.enable_modules(self.engine_config.load_modules.clone());
        } else {
            self.enable_modules(vec![MODULE_TRUNK.to_string()]);
        }

        //load_dynamic_modules();

        LOGGER.debug("Initializing enabled modules");

        self.init_modules();

        LOGGER.info("Engine initialized!");
    }

    pub fn start(&mut self, game_loop: DeltaCallback) {
        LOGGER.info("Bringing up engine");

        LOGGER.fatal_if(!self.is_init_done(), "Cannot start engine before it is initialized.");

        /*LOGGER.fatal_if(!get_client_id().empty(), "Client ID must be set prior to engine start");
        LOGGER.fatal_if(!get_client_name().empty(), "Client ID must be set prior to engine start");
        LOGGER.fatal_if(!get_client_version().empty(), "Client ID must be set prior to engine start");*/

        self.register_update_callback(game_loop);

        let target_fps = self.engine_config.target_framerate;
        let running_state_copy = self.running_state.clone();
        let render_callbacks = self.callbacks.render_callbacks.clone();
        let render_event_queue = self.event_state.render_event_queue.clone();
        let render_event_listeners = self.event_state.render_event_listeners.clone();

        let g_render_thread_jh = std::thread::spawn(move || render_loop(target_fps,
            running_state_copy, render_callbacks, render_event_queue, render_event_listeners));

        //ctrlc::set_handler(|| self.stop_engine());

        LOGGER.info("Engine started! Passing control to game loop.");

        // pass control over to the game loop
        self.game_loop();

        g_render_thread_jh.join();

        LOGGER.info("Game loop has halted, exiting program");

        exit(0);
    }

    pub fn stop_engine(&mut self) {
        if *self.running_state.force_shutdown_on_next_interrupt.lock().unwrap() {
            LOGGER.info("Forcibly terminating process");
            exit(0);
        } else if *self.running_state.game_thread_acknowledged_halt.lock().unwrap() {
            LOGGER.info("Forcibly proceeding with engine bring-down");
            *self.running_state.force_shutdown_on_next_interrupt.lock().unwrap() = true;

            // bit of a hack to trick the game thread into thinking the render thread halted
            let (lock, cvar) = &*self.running_state.render_thread_halted;
            let mut halted = lock.lock().unwrap();
            *halted = true;
            cvar.notify_one();
        } else if *self.running_state.is_stopping.lock().unwrap() {
            LOGGER.warn("Engine is already halting");
        }

        LOGGER.info("Engine halt requested");

        LOGGER.fatal_if(!self.is_init_done(), "Cannot stop engine before it is initialized.");

        *self.running_state.is_stopping.lock().unwrap() = true;
    }

    pub fn is_preinit_done(&self) -> bool {
        self.prev_stage.map(|s| s >= LifecycleStage::PreInit).unwrap_or(false)
    }

    pub fn is_init_done(&self) -> bool {
        println!("Stage: {:?}", self.prev_stage);
        self.prev_stage.map(|s| s >= LifecycleStage::Init).unwrap_or(false)
    }

    pub fn register_update_callback(&mut self, callback: DeltaCallback) -> Index {
        LOGGER.fatal_if(!self.is_init_done(), "Cannot register update callback before engine initialization.");
        return self.callbacks.update_callbacks.write().unwrap().add(callback);
    }

    pub fn unregister_update_callback(&mut self, id: Index) {
        self.callbacks.update_callbacks.write().unwrap().remove(id);
    }

    pub fn register_render_callback(&mut self, callback: DeltaCallback) -> Index {
        LOGGER.fatal_if(!self.is_init_done(), "Cannot register render callback before engine initialization.");
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
                LOGGER.debug("Engine halt request is acknowledged game thread");
                *self.running_state.game_thread_acknowledged_halt.lock().unwrap() = true;

                // wait for render thread to finish up what it's doing so we don't interrupt it ~~and cause a segfault~~
                // edit: lol rust ftw
                {
                    let (lock, cvar) = &*self.running_state.render_thread_halted;
                    if !(*lock.lock().unwrap()) {
                        LOGGER.debug("Game thread observed render thread was not halted, waiting on monitor (send SIGINT again to force halt)");
                        cvar.wait(lock.lock().unwrap());
                    }
                }

                // at this point all event and callback execution should have
                // stopped which allows us to start doing non-thread-safe things

                LOGGER.debug("Game thread observed render thread is halted, proceeding with engine bring-down");

                LOGGER.debug("Deinitializing engine modules");

                self.deinit_modules();

                LOGGER.debug("Deinitializing event callbacks");

                // if we don't do this explicitly, the callback lists (and thus
                // the callback function objects) will be deinitialized
                // statically and will segfault on handlers registered by
                // external libraries (which will have already been unloaded)
                self.deinit_event_handlers();

                LOGGER.debug("Deinitializing general callbacks");

                // same deal here
                self.deinit_callbacks();

                LOGGER.debug("Unloading dynamic engine modules");

                self.unload_dynamic_modules();

                LOGGER.info("Engine bring-down completed");

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
            self.event_state.update_event_listeners.write().unwrap().flush();

            // invoke update callbacks
            for callback in &self.callbacks.update_callbacks.read().unwrap().list {
                (callback.value)(delta);
            }

            process_event_queue(&self.event_state.update_event_queue, &self.event_state.update_event_listeners);

            if let Some(tickrate) = self.engine_config.target_tickrate {
                handle_idle(update_start, tickrate);
            }
        }
    }
}

fn render_loop(target_framerate: Option<u32>, running_state: RunningState,
        render_callbacks: Arc<RwLock<CallbackList<DeltaCallback>>>,
        event_queue: Arc<Mutex<VecDeque<Arc<dyn ArgusEvent + Send + Sync>>>>,
        event_listeners: Arc<RwLock<CallbackList<ArgusEventHandler>>>) {
    let mut last_frame: Option<Instant> = None;

    loop {
        if *running_state.is_stopping.lock().unwrap() {
            LOGGER.debug("Engine halt request is acknowledged by render thread");
            let (lock, cvar) = &*running_state.render_thread_halted;
            let mut halted = lock.lock().unwrap();
            *halted = true;
            cvar.notify_one();
            break;
        }

        let render_start = Instant::now();
        let delta = compute_delta(&mut last_frame);

        render_callbacks.write().unwrap().flush();
        event_listeners.write().unwrap().flush();

        // invoke render callbacks
        for callback in &render_callbacks.read().unwrap().list {
            (callback.value)(delta);
        }

        process_event_queue(&event_queue, &event_listeners);

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
