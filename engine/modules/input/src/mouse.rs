use std::collections::HashMap;
use std::ops::Deref;
use bitflags::bitflags;
use argus_logging::warn;
use lazy_static::lazy_static;
use argus_scripting_bind::script_bind;
use argus_util::math::Vector2d;
use argus_wm::{Window, WindowManager};
use crate::input_event::{dispatch_axis_event, dispatch_button_event};
use crate::{InputManager, LOGGER};

use sdl3::event::Event as SdlEvent;
use sdl3::mouse::MouseButton as SdlMouseButton;
use sdl3::mouse::MouseState as SdlMouseState;

lazy_static! {
    static ref MOUSE_BUTTON_MAP_ARGUS_TO_SDL: HashMap<MouseButton, SdlMouseButton> = HashMap::from([
        (MouseButton::Primary, SdlMouseButton::Left),
        (MouseButton::Middle, SdlMouseButton::Middle),
        (MouseButton::Secondary, SdlMouseButton::Right),
        (MouseButton::Back, SdlMouseButton::X1),
        (MouseButton::Forward, SdlMouseButton::X2),
    ]);
    static ref MOUSE_BUTTON_MAP_SDL_TO_ARGUS: HashMap<SdlMouseButton, MouseButton> =
        MOUSE_BUTTON_MAP_ARGUS_TO_SDL.iter().map(|(k, v)| (*v, *k)).collect();
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[script_bind]
pub enum MouseButton {
    Primary = 1,
    Secondary = 2,
    Middle = 3,
    Back = 4,
    Forward = 5,
}

impl MouseButton {
    pub(crate) fn mask(&self) -> MouseButtonMask {
        match self {
            MouseButton::Primary => MouseButtonMask::Primary,
            MouseButton::Secondary => MouseButtonMask::Secondary,
            MouseButton::Middle => MouseButtonMask::Middle,
            MouseButton::Back => MouseButtonMask::Back,
            MouseButton::Forward => MouseButtonMask::Forward,
        }
    }
}

bitflags! {
    #[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
    pub struct MouseButtonMask: u32 {
        const Primary = 0b0000_0001;
        const Secondary = 0b0000_0010;
        const Middle = 0b0000_0100;
        const Back = 0b0000_1000;
        const Forward = 0b0001_0000;
    }
}

impl From<SdlMouseState> for MouseButtonMask {
    fn from(value: SdlMouseState) -> Self {
        let mut mask = MouseButtonMask::empty();

        if value.left() {
            mask.set(MouseButtonMask::Primary, true);
        }
        if value.right() {
            mask.set(MouseButtonMask::Secondary, true);
        }
        if value.middle() {
            mask.set(MouseButtonMask::Middle, true);
        }
        if value.x1() {
            mask.set(MouseButtonMask::Back, true);
        }
        if value.x2() {
            mask.set(MouseButtonMask::Forward, true);
        }

        mask
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[script_bind]
pub enum MouseAxis {
    Horizontal,
    Vertical,
}

/// Returns the change in position of the mouse from the previous frame.
#[script_bind]
pub fn mouse_delta() -> Vector2d {
    let mouse_state = InputManager::instance().mouse_state.read();
    mouse_state.delta
}

/// Gets the current position of the mouse within the currently focused window.
#[script_bind]
pub fn mouse_pos() -> Vector2d {
    let mouse_state = InputManager::instance().mouse_state.read();
    mouse_state.last_pos.unwrap_or_default()
}

// Gets the current value of a mouse axis.
#[script_bind]
pub fn get_mouse_axis(axis: &MouseAxis) -> f64 {
    let mouse_state = InputManager::instance().mouse_state.read();
    match axis {
        MouseAxis::Horizontal => mouse_state.last_pos.map(|v| v.x),
        MouseAxis::Vertical => mouse_state.last_pos.map(|v| v.y),
    }.unwrap_or_default()
}

/// Gets the last delta of a mouse axis.
#[script_bind]
pub fn get_mouse_axis_delta(axis: MouseAxis) -> f64 {
    let mouse_state = InputManager::instance().mouse_state.read();
    match axis {
        MouseAxis::Horizontal => mouse_state.delta.x,
        MouseAxis::Vertical => mouse_state.delta.y,
    }
}

/// Gets whether a mouse button is currently being pressed.
#[script_bind]
pub fn is_mouse_button_pressed(button: MouseButton) -> bool {
    let mouse_state = InputManager::instance().mouse_state.read();
    mouse_state.button_state.contains(button.mask())
}

fn poll_mouse() {
    let mut mouse_state = InputManager::instance().mouse_state.write();

    let new_state =
        SdlMouseState::new(WindowManager::instance().get_sdl_event_pump().unwrap().deref());
    let new_x = new_state.x();
    let new_y = new_state.y();

    if let Some(last_pos) = mouse_state.last_pos {
        mouse_state.delta.x += new_x as f64 - last_pos.x;
        mouse_state.delta.y += new_y as f64 - last_pos.y;
    }

    mouse_state.last_pos = Some(Vector2d::new(new_x as f64, new_y as f64))
}

fn dispatch_mouse_button_event(
    window: &Window,
    button: &MouseButton,
    release: bool
) {
    for item in &InputManager::instance().controllers {
        let controller_name = item.key();
        let controller = item.value();

        let Some(actions) = controller.get_mouse_button_actions(button) else {
            continue;
        };

        for action in actions {
            dispatch_button_event(
                Some(window.get_id().to_string()),
                controller_name.clone(),
                action,
                release,
            );
        }
    }
}

fn dispatch_mouse_axis_events(window: &Window, x: f64, y: f64, dx: f64, dy: f64) {
    for item in &InputManager::instance().controllers {
        let controller_name = item.key();
        let controller = item.value();
        if let Some(actions_h) = controller.get_mouse_axis_actions(&MouseAxis::Horizontal) {
            for action in actions_h {
                dispatch_axis_event(
                    Some(window.get_id().to_string()),
                    controller_name.clone(),
                    action,
                    x,
                    dx,
                );
            }
        }

        if let Some(actions_v) = controller.get_mouse_axis_actions(&MouseAxis::Vertical) {
            for action in actions_v {
                dispatch_axis_event(
                    Some(window.get_id().to_string()),
                    controller_name.clone(),
                    action,
                    y,
                    dy,
                );
            }
        }
    }
}

fn handle_mouse_events() {
    let event_ss = WindowManager::instance().get_sdl_event_ss()
        .expect("SDL is not yet initialized");
    let events: Vec<SdlEvent> = event_ss.peek_events::<Vec<SdlEvent>>(1024);
    for event in events {
        match event {
            SdlEvent::MouseMotion { window_id, x, y, xrel, yrel, .. } => {
                let Some(window) = WindowManager::instance()
                    .get_window_from_handle_id(window_id) else { continue; };
                dispatch_mouse_axis_events(
                    &window,
                    x as f64,
                    y as f64,
                    xrel as f64,
                    yrel as f64,
                );
            }
            SdlEvent::MouseButtonDown { window_id, mouse_btn, .. } => {
                let Some(button) = MOUSE_BUTTON_MAP_SDL_TO_ARGUS.get(&mouse_btn) else {
                    warn!(LOGGER, "Ignoring unrecognized mouse button {:?}", mouse_btn);
                    return;
                };
                let Some(window) = WindowManager::instance()
                    .get_window_from_handle_id(window_id) else { continue; };
                dispatch_mouse_button_event(
                    &window,
                    button,
                    false,
                );
            }
            SdlEvent::MouseButtonUp { window_id, mouse_btn, .. } => {
                let Some(button) = MOUSE_BUTTON_MAP_SDL_TO_ARGUS.get(&mouse_btn) else {
                    warn!(LOGGER, "Ignoring unrecognized mouse button {:?}", mouse_btn);
                    return;
                };
                let Some(window) = WindowManager::instance()
                    .get_window_from_handle_id(window_id) else { continue; };
                dispatch_mouse_button_event(
                    &window,
                    button,
                    true,
                );
            }
            _ => {}
        }
    }
}

pub(crate) fn init_mouse(_window: &Window) {
    // no-op
}

pub(crate) fn update_mouse() {
    poll_mouse();
    handle_mouse_events();
}

pub(crate) fn flush_mouse_delta() {
    let mut mouse_state = InputManager::instance().mouse_state.write();
    mouse_state.delta = Vector2d::new(0.0, 0.0);
}
