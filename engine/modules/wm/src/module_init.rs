use std::cell::{RefCell};
use std::sync::OnceLock;
use std::thread;
use std::time::{Duration, Instant};
use argus_core::*;
use argus_logging::{crate_logger, debug, info};
use crate::display::init_display;
use crate::{WindowEvent, WindowManager};
use crate::config::WindowConfig;

pub const WINDOWING_MODE_WINDOWED: &str = "windowed";
pub const WINDOWING_MODE_BORDERLESS: &str = "borderless";
pub const WINDOWING_MODE_FULLSCREEN: &str = "fullscreen";

const CONFIG_KEY_WINDOW: &str = "window";

crate_logger!(LOGGER, "argus/wm");

#[allow(non_upper_case_globals)]
static g_is_wm_module_initialized: OnceLock<bool> = OnceLock::new();
thread_local! {
    #[allow(non_upper_case_globals)]
    static g_did_request_stop: RefCell<bool> = const { RefCell::new(false) };
}

fn clean_up() {
    //sdl_quit_subsystem(*ENGINE_SDL_INIT_FLAGS);
    //sdl_quit();
}

fn poll_events() {
    WindowManager::instance().handle_sdl_window_events().expect("Failed to poll SDL events");
}

fn do_window_loop(delta: Duration) {
    WindowManager::instance().update_windows(delta);
    poll_events();
}

fn create_initial_window() {
    debug!(LOGGER, "Creating initial window");

    let config = EngineManager::instance().get_config();
    let params = config.get_section::<WindowConfig>().unwrap();
    //let params = get_initial_window_parameters();
    let Some(id) = &params.id else { return; };
    if id.is_empty() {
        return;
    }

    let mut window = WindowManager::instance().create_window(id)
        .expect("Failed to create initial window");

    if let Some(title) = &params.title {
        window.set_title(title);
    }

    if let Some(mode) = &params.mode {
        if mode == WINDOWING_MODE_WINDOWED {
            window.set_fullscreen(false);
        } else if mode == WINDOWING_MODE_BORDERLESS {
            //TODO
        } else if mode == WINDOWING_MODE_FULLSCREEN {
            window.set_fullscreen(true);
        }
    }

    if let Some(vsync) = params.vsync {
        window.set_vsync_enabled(vsync);
    }

    if let Some(mouse_visible) = params.mouse_visible {
        window.set_mouse_visible(mouse_visible);
    }

    if let Some(mouse_captured) = params.mouse_captured {
        window.set_mouse_captured(mouse_captured);
    }

    if let Some(mouse_raw_input) = params.mouse_raw_input {
        window.set_mouse_raw_input(mouse_raw_input);
    }

    let position = params.position;
    if let Some(position) = position {
        window.set_windowed_position(position.x, position.y);
    }

    let dimensions = params.dimensions;
    window.set_windowed_resolution(dimensions.x, dimensions.y);

    debug!(LOGGER, "Committing initial window '{}'", window.get_id());
    window.commit();
}

fn check_window_count(_delta: Duration) {
    //TODO: make this behavior configurable
    let window_count = WindowManager::instance().get_window_count();
    if window_count == 0 && !g_did_request_stop.replace(true) {
        g_did_request_stop.set(true);
        run_on_update_thread(Box::new(stop_engine), Ordering::Standard);
    }
}

#[register_module(id = "wm", depends(core, scripting))]
pub fn update_lifecycle_wm(stage: LifecycleStage) {
    match stage {
        LifecycleStage::Load => {
        }
        LifecycleStage::PreInit => {
            EngineManager::instance()
                .add_config_deserializer::<WindowConfig>(CONFIG_KEY_WINDOW);
            set_render_loop(render_loop).expect("Failed to set engine render loop");
        }
        LifecycleStage::Init => {
            EngineManager::instance().add_render_init_callback(|| {
                if cfg!(target_os = "linux") {
                    sdl3::hint::set(sdl3::hint::names::VIDEO_DRIVER, "wayland,x11");
                }
                let client_name = EngineManager::instance().get_config()
                    .get_section::<ClientConfig>().unwrap().name.clone();
                sdl3::hint::set(
                    sdl3::hint::names::APP_NAME,
                    &client_name,
                );
                WindowManager::instance().init_sdl().expect("Failed to initialize SDL");
                info!(LOGGER, "SDL initialized successfully");

                init_display();
            }, Ordering::Earliest);

            register_update_callback(Box::new(check_window_count), Ordering::Standard);
            //register_render_callback(Box::new(do_window_loop), Ordering::Standard);
            register_event_handler::<WindowEvent>(
                |event| WindowManager::instance().handle_engine_window_event(event),
                TargetThread::Render,
                Ordering::Standard,
            );

            g_is_wm_module_initialized.set(true).unwrap();
        }
        LifecycleStage::PostInit => {
            // reap any windows created during render module init
            WindowManager::instance().reap_windows();

            EngineManager::instance().add_render_init_callback(
                create_initial_window,
                Ordering::Late,
            );
        }
        LifecycleStage::Deinit => {
            clean_up();

            debug!(LOGGER, "Finished deinitializing wm");
        }
        _ => {}
    }
}

fn render_loop(params: RenderLoopParams) {
    let mut last_time = Instant::now();

    loop {
        if params.should_shutdown() {
            return;
        }
        
        let cur_time = Instant::now();
        let delta = cur_time - last_time;
        last_time = cur_time;

        params.run_core_callbacks(delta);

        do_window_loop(delta);

        let config = EngineManager::instance().get_config();
        if let Some(target_fps) = config.get_section::<CoreConfig>().unwrap().target_framerate {
            if config.get_section::<WindowConfig>().unwrap().vsync.unwrap_or(false) {
                continue;
            }
            if target_fps == 0 {
                continue;
            }
            let target_frame_dur = Duration::from_micros((1000000.0 / target_fps as f32) as u64);
            let cur_tick_dur = Instant::now() - last_time + Duration::from_micros(60);
            if cur_tick_dur >= target_frame_dur {
                continue;
            }
            let sleep_dur = target_frame_dur - cur_tick_dur;
            thread::sleep(sleep_dur);
        }
    }
}

pub fn is_wm_module_initialized() -> bool {
    g_is_wm_module_initialized.get().is_some()
}
