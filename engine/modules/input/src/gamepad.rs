use crate::input_event::{dispatch_axis_event, dispatch_button_event};
use crate::input_manager::GamepadDevicesState;
use crate::{InputDeviceEvent, InputDeviceEventType, InputManager, LOGGER};
use argus_core::dispatch_event;
use argus_logging::{debug, info, warn};
use argus_scripting_bind::script_bind;
use lazy_static::lazy_static;
use num_enum::IntoPrimitive;
use std::cmp::min;
use std::collections::HashMap;

use argus_wm::WindowManager;
use sdl3::event::Event as SdlEvent;
use sdl3::gamepad::Axis as SdlGamepadAxis;
use sdl3::gamepad::Button as SdlGamepadButton;

const GAMEPAD_NAME_INVALID: &str = "invalid";

pub type HidDeviceInstanceId = u32;

type GamepadButtonState = u64;

static AXIS_PAIRS: [(GamepadAxis, GamepadAxis); 3] = [
    (GamepadAxis::LeftX, GamepadAxis::LeftY),
    (GamepadAxis::RightX, GamepadAxis::RightY),
    (GamepadAxis::LTrigger, GamepadAxis::RTrigger),
];

lazy_static! {
    static ref BUTTON_MAP_ARGUS_TO_SDL:
    HashMap<GamepadButton, SdlGamepadButton> = HashMap::from([
        (GamepadButton::A, SdlGamepadButton::South),
        (GamepadButton::B, SdlGamepadButton::East),
        (GamepadButton::X, SdlGamepadButton::West),
        (GamepadButton::Y, SdlGamepadButton::North),
        (GamepadButton::DpadUp, SdlGamepadButton::DPadUp),
        (GamepadButton::DpadDown, SdlGamepadButton::DPadDown),
        (GamepadButton::DpadLeft, SdlGamepadButton::DPadLeft),
        (GamepadButton::DpadRight, SdlGamepadButton::DPadRight),
        (GamepadButton::LBumper, SdlGamepadButton::LeftShoulder),
        (GamepadButton::RBumper, SdlGamepadButton::RightShoulder),
        (GamepadButton::LStick, SdlGamepadButton::LeftStick),
        (GamepadButton::RStick, SdlGamepadButton::RightStick),
        (GamepadButton::L4, SdlGamepadButton::LeftPaddle1),
        (GamepadButton::R4, SdlGamepadButton::RightPaddle1),
        (GamepadButton::L5, SdlGamepadButton::LeftPaddle2),
        (GamepadButton::R5, SdlGamepadButton::RightPaddle2),
        (GamepadButton::Start, SdlGamepadButton::Start),
        (GamepadButton::Back, SdlGamepadButton::Back),
        (GamepadButton::Guide, SdlGamepadButton::Guide),
        (GamepadButton::Misc1, SdlGamepadButton::Misc1),
    ]);
    static ref BUTTON_MAP_SDL_TO_ARGUS: HashMap<SdlGamepadButton, GamepadButton> =
        BUTTON_MAP_ARGUS_TO_SDL.iter().map(|(k, v)| (*v, *k)).collect();

    static ref AXIS_MAP_ARGUS_TO_SDL:
    HashMap<GamepadAxis, SdlGamepadAxis> = HashMap::from([
        (GamepadAxis::LeftX, SdlGamepadAxis::LeftX),
        (GamepadAxis::LeftY, SdlGamepadAxis::LeftY),
        (GamepadAxis::RightX, SdlGamepadAxis::RightX),
        (GamepadAxis::RightY, SdlGamepadAxis::RightY),
        (GamepadAxis::LTrigger, SdlGamepadAxis::TriggerLeft),
        (GamepadAxis::RTrigger, SdlGamepadAxis::TriggerRight),
    ]);
    static ref AXIS_MAP_SDL_TO_ARGUS: HashMap<SdlGamepadAxis, GamepadAxis> =
        AXIS_MAP_ARGUS_TO_SDL.iter().map(|(k, v)| (*v, *k)).collect();
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[script_bind]
pub enum GamepadButton {
    Unknown = -1,
    A,
    B,
    X,
    Y,
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,
    LBumper,
    RBumper,
    LTrigger,
    RTrigger,
    LStick,
    RStick,
    L4,
    R4,
    L5,
    R5,
    Start,
    Back,
    Guide,
    Misc1,
}

impl GamepadButton {
    pub fn values() -> &'static [GamepadButton] {
        &[
            Self::Unknown,
            Self::A,
            Self::B,
            Self::X,
            Self::Y,
            Self::DpadUp,
            Self::DpadDown,
            Self::DpadLeft,
            Self::DpadRight,
            Self::LBumper,
            Self::RBumper,
            Self::LTrigger,
            Self::RTrigger,
            Self::LStick,
            Self::RStick,
            Self::L4,
            Self::R4,
            Self::L5,
            Self::R5,
            Self::Start,
            Self::Back,
            Self::Guide,
            Self::Misc1,
        ]
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq)]
#[repr(i32)]
#[script_bind]
pub enum GamepadAxis {
    Unknown = -1,
    LeftX,
    LeftY,
    RightX,
    RightY,
    LTrigger,
    RTrigger,
    MaxValue,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[script_bind]
pub enum DeadzoneShape {
    Ellipse,
    Quad,
    Cross,
    MaxValue
}

#[script_bind]
pub fn get_connected_gamepad_count() -> u8 {
    let devices_state = InputManager::instance().gamepad_devices_state.read();
    min(
        devices_state.available_gamepads.len() + devices_state.mapped_gamepads.len(),
        u8::MAX as usize
    ) as u8
}

#[script_bind]
pub fn get_unattached_gamepad_count() -> u8 {
    let devices_state = InputManager::instance().gamepad_devices_state.read();
    min(devices_state.available_gamepads.len(), u8::MAX as usize) as u8
}

#[script_bind]
//TODO: gamepad arg should be HidDeviceInstanceId
pub fn get_gamepad_name(gamepad: u32) -> String {
    let Ok(controller) = InputManager::instance().get_sdl_gamepad_ss().unwrap().open(gamepad) else {
        warn!(LOGGER, "Client queried unknown gamepad ID {}", gamepad);
        return GAMEPAD_NAME_INVALID.to_string();
    };

    controller.name().unwrap_or("(unknown)".to_string())
}

#[script_bind]
//TODO: gamepad arg should be HidDeviceInstanceId
pub fn is_gamepad_button_pressed(gamepad: u32, button: GamepadButton) -> bool {
    let Some(sdl_button) = BUTTON_MAP_ARGUS_TO_SDL.get(&button) else {
        warn!(LOGGER, "Client polled unknown gamepad button {:?}", button);
        return false;
    };

    let Some(gamepad_state) = InputManager::instance().gamepad_states.get(&gamepad) else {
        warn!(LOGGER, "Client polled unknown gamepad ID {}", gamepad);
        return false;
    };

    let sdl_button_ordinal: i32 = sdl_button.to_ll().0 as i32;
    assert!(sdl_button_ordinal < 64);
    (gamepad_state.button_state & (1 << sdl_button_ordinal as u8)) != 0
}

#[script_bind]
//TODO: gamepad arg should be HidDeviceInstanceId
pub fn get_gamepad_axis(gamepad: u32, axis: GamepadAxis) -> f64 {
    let Some(gamepad_state) = InputManager::instance().gamepad_states.get(&gamepad) else {
        warn!(LOGGER, "Client polled unknown gamepad ID {}", gamepad);
        return 0.0;
    };

    gamepad_state.axis_state[&axis]
}

#[script_bind]
//TODO: gamepad arg should be HidDeviceInstanceId
pub fn get_gamepad_axis_delta(gamepad: u32, axis: GamepadAxis) -> f64 {
    let Some(gamepad_state) = InputManager::instance().gamepad_states.get(&gamepad) else {
        warn!(LOGGER, "Client polled unknown gamepad ID {}", gamepad);
        return 0.0;
    };

    gamepad_state.axis_state[&axis]
}

fn init_gamepads(devices_state: &mut GamepadDevicesState) {
    for joystick_id in InputManager::instance().get_sdl_joystick_ss().unwrap().joysticks()
        .expect("Failed to enumerate joysticks") {
        let Ok(gamepad) = InputManager::instance().get_sdl_gamepad_ss().unwrap()
            .open(joystick_id) else {
            debug!(LOGGER, "Joystick {} is not reported as a gamepad, ignoring", joystick_id);
            continue;
        };
        let name = gamepad.name();
        info!(LOGGER, "Opening joystick '{}' as a gamepad", name.unwrap_or("(unknown)".to_string()));
        InputManager::instance().get_sdl_gamepad_ss().unwrap().open(gamepad.id().unwrap())
            .expect("Failed to open joystick as game controller");
        devices_state.available_gamepads.push(gamepad.id().unwrap());
    }

    let gamepad_count = devices_state.available_gamepads.len();
    if gamepad_count > 0 {
        info!(
            LOGGER,
            "{} connected gamepad{} found",
            gamepad_count,
            if gamepad_count != 1 { "s" } else { "" }
        );
    } else {
        info!(LOGGER, "No gamepads connected");
    }
}

fn normalize_axis(val: i16) -> f64 {
    if val == 0 {
        0.0
    } else if val >= 0 {
        (val as f64) / i16::MAX as f64
    } else {
        (val as f64) / i16::MIN as f64
    }
}

fn poll_gamepad(devices_state: &mut GamepadDevicesState, id: HidDeviceInstanceId) {
    let Ok(gamepad) = InputManager::instance().get_sdl_gamepad_ss().unwrap().open(id) else {
        warn!(LOGGER, "Failed to get SDL controller from instance ID {}", id);
        return;
    };

    let controller_opt = devices_state.mapped_gamepads.get(&id)
        .map(|name| InputManager::instance().get_controller(name));

    let mut new_button_state: GamepadButtonState = 0;
    let mut i = 0;
    for button in GamepadButton::values() {
        let sdl_button = BUTTON_MAP_ARGUS_TO_SDL.get(&button).unwrap();
        let bit = if gamepad.button(*sdl_button) { 1 } else { 0 };
        new_button_state |= bit << i;
        i += 1;
    }

    let mut new_axis_state = HashMap::new();

    for (axis_1, axis_2) in AXIS_PAIRS {
        let axis_1_val = normalize_axis(gamepad.axis(AXIS_MAP_ARGUS_TO_SDL[&axis_1]));
        let axis_2_val = normalize_axis(gamepad.axis(AXIS_MAP_ARGUS_TO_SDL[&axis_2]));

        let (shape, radius_x, radius_y) = match &controller_opt {
            Some(controller) => (
                controller.get_axis_deadzone_shape(axis_1),
                controller.get_axis_deadzone_radius(axis_1),
                controller.get_axis_deadzone_radius(axis_2),
            ),
            None => (
                InputManager::instance().get_global_axis_deadzone_shape(&axis_1),
                InputManager::instance().get_global_axis_deadzone_radius(&axis_1),
                InputManager::instance().get_global_axis_deadzone_radius(&axis_2),
            ),
        };

        if radius_x == 0.0 || radius_y == 0.0 {
            continue;
        }

        let x = axis_1_val;
        let y = axis_2_val;

        let x2 = x.powf(2.0);
        let y2 = y.powf(2.0);

        // distance from the origin to the bounding box along angle theta
        // (where theta = atan2(x, y))
        let d_boundary = if x.abs() < y.abs() {
            (1.0 + x2 / (x2 + y2)).sqrt()
        } else if x.abs() == y.abs() {
            2.0f64.sqrt() // degenerate case where we use distance to corner
        } else {
            (1.0f64 + y2 / (x2 + y2)).sqrt()
        };

        // distance from the origin to the point
        let d_center = (x2 + y2).sqrt();

        // distance from the origin to the edge of the deadzone at angle theta
        let r_deadzone;

        let new_x: f64;
        let new_y: f64;
        match shape {
            DeadzoneShape::Ellipse => {
                let a2 = radius_x.powf(2.0);
                let b2 = radius_y.powf(2.0);
                if x < radius_x &&
                    y < radius_y &&
                    x2 / a2 + y2 / b2 <= 1.0 {
                    new_x = 0.0;
                    new_y = 0.0;
                } else {
                    if (radius_x.abs() - radius_y.abs()).abs() <= f64::EPSILON {
                        // it's a circle so literally just use the constant radius
                        r_deadzone = radius_x;
                    } else {
                        // "radius" of ellipse changes with theta so we need to compute it
                        r_deadzone = (2.0 * a2 * b2 - a2 * y2 - b2 * x2).abs().sqrt();
                    }

                    let d_deadzone_to_point = d_center - r_deadzone;
                    let d_deadzone_to_boundary = d_boundary - r_deadzone;

                    assert!(d_deadzone_to_boundary > 0.0);
                    new_x = x * (d_deadzone_to_point / d_deadzone_to_boundary);
                    new_y = y * (d_deadzone_to_point / d_deadzone_to_boundary);
                }
            }
            DeadzoneShape::Quad => {
                if x.abs() < radius_x && y.abs() < radius_y {
                    new_x = 0.0;
                    new_y = 0.0;
                } else {
                    assert!(radius_x < 1.0);
                    assert!(radius_y < 1.0);
                    let r = x.abs().max(y.abs());
                    new_x = x * (r - radius_x) / (1.0 - radius_x);
                    new_y = y * (r - radius_y) / (1.0 - radius_y);
                }
            }
            DeadzoneShape::Cross => {
                if x.abs() < radius_x {
                    new_x = 0.0;
                } else {
                    assert!(radius_x < 1.0);
                    new_x = x * (x.abs() - radius_x) / (1.0 - radius_x);
                }
                if y.abs() < radius_y {
                    new_y = 0.0;
                } else {
                    assert!(radius_y < 1.0);
                    new_y = y * (y.abs() - radius_y) / (1.0 - radius_y);
                }
            }
            _ => {
                warn!(LOGGER, "Ignoring unhandled deadzone shape {:?}", shape);
                continue;
            }
        }

        new_axis_state.insert(axis_1, new_x);
        new_axis_state.insert(axis_2, new_y);
    }

    let mut gamepad_state = InputManager::instance().gamepad_states.get_mut(&id).unwrap();

    let prev_axis_state = &gamepad_state.axis_state.clone();

    gamepad_state.button_state = new_button_state as u64;
    for (axis, val) in &new_axis_state {
        gamepad_state.axis_deltas.insert(*axis, val - prev_axis_state[axis]);
    }
    gamepad_state.axis_state = new_axis_state;
}

fn dispatch_button_events(button: &GamepadButton, release: bool) {
    for item in &InputManager::instance().controllers {
        let controller_name = item.key();
        let controller = item.value();

        let Some(actions) = controller.get_gamepad_button_actions(button) else {
            continue;
        };

        for action in actions {
            dispatch_button_event(None, controller_name.clone(), action, release);
        }
    }
}

fn dispatch_axis_events(axis: &GamepadAxis, val: f64, delta: f64) {
    for item in &InputManager::instance().controllers {
        let controller_name = item.key();
        let controller = item.value();

        let Some(actions) = controller.get_gamepad_axis_actions(axis) else {
            continue;
        };
        for action in actions {
            dispatch_axis_event(None, controller_name.clone(), action, val, delta);
        }
    }
}

fn dispatch_gamepad_connect_event(gamepad_id: HidDeviceInstanceId) {
    dispatch_event::<InputDeviceEvent>(InputDeviceEvent::new(
        InputDeviceEventType::GamepadConnected,
        "",
        gamepad_id
    ));
}

fn dispatch_gamepad_disconnect_event(
    controller_name: impl Into<String>,
    gamepad_id: HidDeviceInstanceId
) {
    dispatch_event::<InputDeviceEvent>(InputDeviceEvent::new(
        InputDeviceEventType::GamepadDisconnected,
        controller_name,
        gamepad_id
    ));
}

fn handle_gamepad_events(devices_state: &mut GamepadDevicesState) {
    let event_ss = WindowManager::instance().get_sdl_event_ss()
        .expect("SDL is not yet initialized");
    let events: Vec<SdlEvent> = event_ss.peek_events::<Vec<SdlEvent>>(1024);
    for event in events {
        match event {
            SdlEvent::ControllerButtonDown { button, .. } => {
                let Some(engine_button) = BUTTON_MAP_SDL_TO_ARGUS.get(&button) else {
                    warn!(LOGGER, "Ignoring event for unknown gamepad button {:?}", button);
                    continue;
                };
                dispatch_button_events(engine_button, false);
            }
            SdlEvent::ControllerButtonUp { button, .. } => {
                let Some(engine_button) = BUTTON_MAP_SDL_TO_ARGUS.get(&button) else {
                    warn!(LOGGER, "Ignoring event for unknown gamepad button {:?}", button);
                    continue;
                };
                dispatch_button_events(engine_button, true);
            }
            SdlEvent::ControllerAxisMotion { axis, value, .. } => {
                let Some(sdl_axis) = AXIS_MAP_SDL_TO_ARGUS.get(&axis) else {
                    warn!(LOGGER, "Ignoring event for unknown gamepad axis {:?}", axis);
                    continue;
                };
                //TODO: figure out what to do about the delta
                dispatch_axis_events(sdl_axis, normalize_axis(value), 0.0);
            }
            SdlEvent::ControllerDeviceAdded { which, .. } => {
                let device_index = which;

                let gamepad = InputManager::instance().get_sdl_gamepad_ss().unwrap()
                    .open(device_index).unwrap();
                let instance_id = gamepad.id().unwrap();

                if devices_state.mapped_gamepads.contains_key(&instance_id) ||
                    devices_state.available_gamepads.contains(&instance_id) {
                    warn!(
                        LOGGER,
                        "Ignoring connect event for previously opened gamepad
                            with instance ID {}",
                        instance_id);
                    continue;
                }

                devices_state.available_gamepads.push(instance_id);

                let name = gamepad.name().unwrap_or("(unknown)".to_string());
                warn!(
                    LOGGER,
                    "Gamepad '{}' with instance ID {} was connected",
                    name,
                    instance_id
                );

                dispatch_gamepad_connect_event(instance_id);
            }
            SdlEvent::ControllerDeviceRemoved { which, .. } => {
                let instance_id = which as HidDeviceInstanceId;

                let mut devices_state =
                    InputManager::instance().gamepad_devices_state.write();
                if let Some(controller_name) =
                    devices_state.mapped_gamepads.get(&instance_id) {
                    match InputManager::instance().controllers.get_mut(controller_name) {
                        Some(mut controller) => {
                            info!(
                                LOGGER,
                                "Gamepad attached to controller '{}' was disconnected",
                                controller_name,
                            );
                            controller.was_gamepad_disconnected = true;

                            dispatch_gamepad_disconnect_event(controller_name, instance_id);
                        }
                        None => {
                            // shouldn't happen

                            devices_state.mapped_gamepads.remove(&instance_id);

                            dispatch_gamepad_disconnect_event("", instance_id);
                        }
                    }
                } else {
                    let mut devices_state =
                        InputManager::instance().gamepad_devices_state.write();
                    devices_state.available_gamepads.retain(|id| id != &instance_id);

                    debug!(
                        LOGGER,
                        "Gamepad with instance ID {} was disconnected",
                        instance_id
                    );

                    dispatch_gamepad_disconnect_event("", instance_id);
                }
            }
            _ => { continue; }
        }
    }
}

pub(crate) fn update_gamepads() {
    let mut devices_state = InputManager::instance().gamepad_devices_state.write();

    if !devices_state.are_gamepads_initted {
        init_gamepads(&mut devices_state);

        devices_state.are_gamepads_initted = true;
    }

    handle_gamepad_events(&mut devices_state);

    for gamepad_id in devices_state.available_gamepads.clone() {
        poll_gamepad(&mut devices_state, gamepad_id);
    }

    for (gamepad_id, _) in devices_state.mapped_gamepads.clone() {
        poll_gamepad(&mut devices_state, gamepad_id);
    }
}

pub(crate) fn flush_gamepad_deltas() {
    let gamepad_states = &InputManager::instance().gamepad_states;

    for mut item in gamepad_states.iter_mut() {
        let gamepad = item.value_mut();

        gamepad.axis_deltas.clear();
    }
}

pub(crate) fn assoc_gamepad(id: HidDeviceInstanceId, controller_name: impl Into<String>)
    -> Result<(), String> {
    let mut devices_state = InputManager::instance().gamepad_devices_state.write();
    let gamepads = &mut devices_state.available_gamepads;
    let Some(pos) = gamepads.iter().position(|el| el == &id) else {
        return Err("Gamepad ID is not valid or is already in use".to_string());
    };

    gamepads.remove(pos);
    devices_state.mapped_gamepads.insert(id, controller_name.into());

    Ok(())
}

pub(crate) fn assoc_first_available_gamepad(controller_name: impl Into<String>)
    -> Result<HidDeviceInstanceId, String> {
    let devices_state = InputManager::instance().gamepad_devices_state.write();

    match devices_state.available_gamepads.first() {
        Some(id) => {
            assoc_gamepad(*id, controller_name)?;
            Ok(*id)
        },
        None => Err("No available gamepads".to_string()),
    }
}

pub(crate) fn unassoc_gamepad(id: HidDeviceInstanceId) {
    let mut devices_state = InputManager::instance().gamepad_devices_state.write();

    if devices_state.mapped_gamepads.remove(&id).is_none() {
        warn!(LOGGER, "Client attempted to close unmapped gamepad instance ID {}", id);
        return;
    };

    devices_state.available_gamepads.push(id);
}

fn close_gamepad(_id: HidDeviceInstanceId) {
    //TODO
    /*let Ok(controller) = InputManager::instance().get_sdl_gamepad_ss().unwrap().open(id) else {
        warn!(
            LOGGER,
            "Failed to get SDL gamepad with instance ID {} while deinitializing gamepads",
            id
        );
        return;
    };
    controller.close();*/
}

pub(crate) fn deinit_gamepads() {
    let devices_state = InputManager::instance().gamepad_devices_state.read();

    for id in &devices_state.available_gamepads {
        close_gamepad(*id);
    }

    for id in devices_state.mapped_gamepads.keys() {
        close_gamepad(*id);
    }
}
