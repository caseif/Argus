use std::cell::{RefCell};
use std::sync::{LazyLock, OnceLock};
use std::time::Duration;
use core_rustabi::argus::core::*;
use sdl2::error::sdl_get_error;
use sdl2::events::sdl_pump_events;
use sdl2::hints::{sdl_set_hint, SDL_HINT_APP_NAME, SDL_HINT_VIDEODRIVER};
use sdl2::{sdl_init, sdl_quit, sdl_quit_subsystem, SdlInitFlags};
use crate::display::init_display;
use crate::{WindowEvent, WindowManager};

pub const WINDOWING_MODE_WINDOWED: &str = "windowed";
pub const WINDOWING_MODE_BORDERLESS: &str = "borderless";
pub const WINDOWING_MODE_FULLSCREEN: &str = "fullscreen";

static ENGINE_SDL_INIT_FLAGS: LazyLock<SdlInitFlags> =
    LazyLock::new(|| SdlInitFlags::Events |
        SdlInitFlags::Video |
        SdlInitFlags::GameController);

#[allow(non_upper_case_globals)]
static g_is_wm_module_initialized: OnceLock<bool> = OnceLock::new();
thread_local! {
    #[allow(non_upper_case_globals)]
    static g_did_request_stop: RefCell<bool> = RefCell::new(false);
}

fn clean_up() {
    sdl_quit_subsystem(*ENGINE_SDL_INIT_FLAGS);
    sdl_quit();
}

fn poll_events() {
    sdl_pump_events();

    WindowManager::instance().handle_sdl_window_events();
}

fn do_window_loop(delta: Duration) {
    WindowManager::instance().update_windows(delta);
    poll_events();
}

fn create_initial_window() {
    let params = get_initial_window_parameters();
    let Some(id) = params.id else { return; };
    if id.is_empty() {
        return;
    }

    let mut window = WindowManager::instance().create_window(&id, None);

    if let Some(title) = params.title {
        window.set_title(title);
    }

    if let Some(mode) = params.mode {
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

    if let Some(position) = params.position {
        window.set_windowed_position(position.x, position.y);
    }

    if let Some(dimensions) = params.dimensions {
        window.set_windowed_resolution(dimensions.x, dimensions.y);
    }

    window.commit();
}

fn check_window_count(_delta: Duration) {
    //TODO: make this behavior configurable
    let window_count = WindowManager::instance().get_window_count();
    if window_count == 0 && !g_did_request_stop.replace(true) {
        g_did_request_stop.set(true);
        run_on_game_thread(Box::new(stop_engine));
    }
}

#[no_mangle]
pub unsafe extern "C" fn update_lifecycle_wm_rs(stage: LifecycleStage) {
    match stage {
        LifecycleStage::Init => {
            if cfg!(target_os = "linux") {
                sdl_set_hint(SDL_HINT_VIDEODRIVER, "x11,wayland");
            }
            sdl_set_hint(SDL_HINT_APP_NAME, get_client_name());
            if sdl_init(*ENGINE_SDL_INIT_FLAGS) != 0 {
                panic!("SDL init failed: {}", sdl_get_error().get_message());
            }
            println!("SDL initialized successfully");

            register_update_callback(Box::new(check_window_count), Ordering::Standard);
            register_render_callback(Box::new(do_window_loop), Ordering::Standard);
            register_event_handler::<WindowEvent>(
                Box::new(|event| WindowManager::instance().handle_engine_window_event(event)),
                TargetThread::Render,
                Ordering::Standard,
            );

            init_display();

            g_is_wm_module_initialized.set(true).unwrap();
        }
        LifecycleStage::PostInit => {
            // reap any windows created during render module init
            WindowManager::instance().reap_windows();

            create_initial_window();
        }
        LifecycleStage::Deinit => {
            clean_up();

            println!("Finished deinitializing wm");
        }
        _ => {}
    }
}

pub fn is_wm_module_initialized() -> bool {
    g_is_wm_module_initialized.get().is_some()
}
