use std::collections::HashMap;
use lazy_static::lazy_static;
use wm_rustabi::argus::wm::{get_window_from_handle, Window};
use argus_scripting_bind::script_bind;
use lowlevel_rustabi::argus::lowlevel::Vector2d;
use sdl2::events::{sdl_get_events, SdlEventData, SdlEventType};
use sdl2::mouse::{sdl_get_mouse_state, SdlMouseButton};
use sdl2::video::SdlWindow;
use crate::input_event::{dispatch_axis_event, dispatch_button_event};
use crate::InputManager;

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
pub fn get_mouse_axis_delta(axis: &MouseAxis) -> f64 {
    let mouse_state = InputManager::instance().mouse_state.read();
    match axis {
        MouseAxis::Horizontal => mouse_state.delta.x,
        MouseAxis::Vertical => mouse_state.delta.y,
    }
}

/// Gets whether a mouse button is currently being pressed.
#[script_bind]
pub fn is_mouse_button_pressed(button: &MouseButton) -> bool {
    let Some(sdl_button) = MOUSE_BUTTON_MAP_ARGUS_TO_SDL.get(button) else {
        panic!("Invalid mouse button {:?}", button);
    };
    let mouse_state = InputManager::instance().mouse_state.read();
    (mouse_state.button_state & sdl_button.get_mask() as u32) != 0
}

fn poll_mouse() {
    let mut mouse_state = InputManager::instance().mouse_state.write();

    let new_state = sdl_get_mouse_state();
    let new_x = new_state.position_x;
    let new_y = new_state.position_y;

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
            dispatch_button_event(Some(window), controller_name.clone(), action, release);
        }
    }
}

fn dispatch_mouse_axis_events(window: &Window, x: f64, y: f64, dx: f64, dy: f64) {
    for item in &InputManager::instance().controllers {
        let controller_name = item.key();
        let controller = item.value();
        if let Some(actions_h) = controller.get_mouse_axis_actions(&MouseAxis::Horizontal) {
            for action in actions_h {
                dispatch_axis_event(Some(&window), controller_name.clone(), action, x, dx);
            }
        }

        if let Some(actions_v) = controller.get_mouse_axis_actions(&MouseAxis::Vertical) {
            for action in actions_v {
                dispatch_axis_event(Some(&window), controller_name.clone(), action, y, dy);
            }
        }
    }
}

fn handle_mouse_events() {
    for event in sdl_get_events(SdlEventType::MouseMotion, SdlEventType::MouseButtonUp) {
        match event.data {
            SdlEventData::MouseMotion(data) => {
                let Some(window) = (match SdlWindow::from_id(data.window_id) {
                    Ok(sdl_window) => get_window_from_handle(sdl_window.get_handle()),
                    Err(_) => None,
                }) else { continue; };
                dispatch_mouse_axis_events(
                    &window,
                    data.x as f64,
                    data.y as f64,
                    data.x_rel as f64,
                    data.y_rel as f64,
                );
            }
            SdlEventData::MouseButton(data) => {
                let Some(button) = MOUSE_BUTTON_MAP_SDL_TO_ARGUS.get(&data.button) else {
                    println!("Ignoring unrecognized mouse button {:?}", data.button);
                    return;
                };
                let Some(window) = (match SdlWindow::from_id(data.window_id) {
                    Ok(sdl_window) => get_window_from_handle(sdl_window.get_handle()),
                    Err(_) => None,
                }) else { continue; };
                dispatch_mouse_button_event(
                    &window,
                    button,
                    event.ty == SdlEventType::MouseButtonUp,
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
