#![feature(used_with_arg)]

use std::time::Duration;
use argus_logging::{crate_logger, warn};
use argus_core::{register_event_handler, register_module, register_render_callback, register_update_callback, EngineManager, LifecycleStage, Ordering, TargetThread};
use argus_wm::{Window, WindowEvent, WindowEventType, WindowManager};

mod controller;
mod gamepad;
mod input_event;
mod input_manager;
mod keyboard;
mod mouse;
mod text_input_context;
mod deadzones;
mod config;

pub use input_event::*;
pub use input_manager::*;
use crate::config::BindingsConfig;
use crate::controller::ack_gamepad_disconnects;
use crate::gamepad::{deinit_gamepads, flush_gamepad_deltas, update_gamepads};
use crate::keyboard::{init_keyboard, update_keyboard};
use crate::mouse::{flush_mouse_delta, init_mouse, update_mouse};

crate_logger!(LOGGER, "argus/input");

const CONFIG_KEY_BINDINGS: &str = "bindings";

fn init_window_input(window: &Window) {
    init_keyboard(window);
    init_mouse(window);
}

fn on_window_event(event: &WindowEvent) {
    match event.subtype {
        WindowEventType::Create => {
            let Some(window) = WindowManager::instance().get_window(&event.window)
            else {
                warn!(LOGGER, "Received window event with unknown window ID!");
                return;
            };
            init_window_input(window.value());
        }
        WindowEventType::Focus => {
            //TODO: figure out how to move cursor inside window boundary
        }
        _ => {}
    }
}

fn on_update_early(_delta: Duration) {
    ack_gamepad_disconnects();
}

fn on_update_late(_delta: Duration) {
    flush_mouse_delta();
    flush_gamepad_deltas();
}

fn on_render(_delta: Duration) {
    update_keyboard();
    update_mouse();
    update_gamepads();
}

#[register_module(id = "input", depends(core, scripting, wm))]
pub fn update_lifecycle_input(stage: LifecycleStage) {
    match stage {
        LifecycleStage::PreInit => {
            EngineManager::instance()
                .add_config_deserializer::<BindingsConfig>(CONFIG_KEY_BINDINGS);
        }
        LifecycleStage::Init => {
            register_update_callback(Box::new(on_update_early), Ordering::Early);
            register_update_callback(Box::new(on_update_late), Ordering::Late);
            register_render_callback(Box::new(on_render), Ordering::Early);
            register_event_handler::<WindowEvent>(
                Box::new(on_window_event),
                TargetThread::Render,
                Ordering::Standard,
            );
            
            EngineManager::instance().add_render_init_callback(
                || InputManager::instance().init_sdl(),
                Ordering::Early,
            );
        }
        LifecycleStage::Deinit => {
            deinit_gamepads();
        }
        _ => {}
    }
}
