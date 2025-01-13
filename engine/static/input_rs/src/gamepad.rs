use std::cmp::min;
use std::collections::HashMap;
use core_rustabi::argus::core::dispatch_event;
use lazy_static::lazy_static;
use num_enum::{IntoPrimitive, TryFromPrimitive};
use argus_scripting_bind::script_bind;
use sdl2::events::{sdl_get_events, SdlButtonState, SdlEventData, SdlEventType};
use sdl2::gamecontroller::*;
use sdl2::joystick::*;
use crate::input_event::{dispatch_axis_event, dispatch_button_event};
use crate::{InputDeviceEvent, InputDeviceEventType, InputManager};
use crate::input_manager::GamepadDevicesState;

const GAMEPAD_NAME_INVALID: &str = "invalid";
const GAMEPAD_NAME_UNKNOWN: &str = "unknown";

pub type HidDeviceInstanceId = i32;

type GamepadButtonState = u64;

static AXIS_PAIRS: [(GamepadAxis, GamepadAxis); 3] = [
    (GamepadAxis::LeftX, GamepadAxis::LeftY),
    (GamepadAxis::RightX, GamepadAxis::RightY),
    (GamepadAxis::LTrigger, GamepadAxis::RTrigger),
];

lazy_static! {
    static ref BUTTON_MAP_ARGUS_TO_SDL:
    HashMap<GamepadButton, SdlGameControllerButton> = HashMap::from([
        (GamepadButton::A, SdlGameControllerButton::A),
        (GamepadButton::B, SdlGameControllerButton::B),
        (GamepadButton::X, SdlGameControllerButton::X),
        (GamepadButton::Y, SdlGameControllerButton::Y),
        (GamepadButton::DpadUp, SdlGameControllerButton::DpadUp),
        (GamepadButton::DpadDown, SdlGameControllerButton::DpadDown),
        (GamepadButton::DpadLeft, SdlGameControllerButton::DpadLeft),
        (GamepadButton::DpadRight, SdlGameControllerButton::DpadRight),
        (GamepadButton::LBumper, SdlGameControllerButton::LeftShoulder),
        (GamepadButton::RBumper, SdlGameControllerButton::RightShoulder),
        (GamepadButton::LStick, SdlGameControllerButton::LeftStick),
        (GamepadButton::RStick, SdlGameControllerButton::RightStick),
        (GamepadButton::L4, SdlGameControllerButton::Paddle1),
        (GamepadButton::R4, SdlGameControllerButton::Paddle2),
        (GamepadButton::L5, SdlGameControllerButton::Paddle3),
        (GamepadButton::R5, SdlGameControllerButton::Paddle4),
        (GamepadButton::Start, SdlGameControllerButton::Start),
        (GamepadButton::Back, SdlGameControllerButton::Back),
        (GamepadButton::Guide, SdlGameControllerButton::Guide),
        (GamepadButton::Misc1, SdlGameControllerButton::Misc1),
    ]);
    static ref BUTTON_MAP_SDL_TO_ARGUS: HashMap<SdlGameControllerButton, GamepadButton> =
        BUTTON_MAP_ARGUS_TO_SDL.iter().map(|(k, v)| (*v, *k)).collect();

    static ref AXIS_MAP_ARGUS_TO_SDL:
    HashMap<GamepadAxis, SdlGameControllerAxis> = HashMap::from([
        (GamepadAxis::Unknown, SdlGameControllerAxis::Invalid),
        (GamepadAxis::LeftX, SdlGameControllerAxis::LeftX),
        (GamepadAxis::LeftY, SdlGameControllerAxis::LeftY),
        (GamepadAxis::RightX, SdlGameControllerAxis::RightX),
        (GamepadAxis::RightY, SdlGameControllerAxis::RightY),
        (GamepadAxis::LTrigger, SdlGameControllerAxis::TriggerLeft),
        (GamepadAxis::RTrigger, SdlGameControllerAxis::TriggerRight),
        (GamepadAxis::MaxValue, SdlGameControllerAxis::MaxValue),
    ]);
    static ref AXIS_MAP_SDL_TO_ARGUS: HashMap<SdlGameControllerAxis, GamepadAxis> =
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
    MaxValue,
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
pub fn get_gamepad_name(gamepad: i32) -> String {
    let Ok(controller) = SdlGameController::from_instance_id(gamepad) else {
        println!("Client queried unknown gamepad ID {}", gamepad);
        return GAMEPAD_NAME_INVALID.to_string();
    };

    let Ok(name) = controller.get_name() else {
        println!("Failed to query gamepad name");
        return GAMEPAD_NAME_UNKNOWN.to_string();
    };
    name
}

#[script_bind]
//TODO: gamepad arg should be HidDeviceInstanceId
pub fn is_gamepad_button_pressed(gamepad: i32, button: GamepadButton) -> bool {
    let Some(sdl_button) = BUTTON_MAP_ARGUS_TO_SDL.get(&button) else {
        println!("Client polled unknown gamepad button {:?}", button);
        return false;
    };

    let Some(gamepad_state) = InputManager::instance().gamepad_states.get(&gamepad) else {
        println!("Client polled unknown gamepad ID {}", gamepad);
        return false;
    };

    let sdl_button_ordinal: i32 = (*sdl_button).into();
    assert!(sdl_button_ordinal < 64);
    (gamepad_state.button_state & (1 << sdl_button_ordinal as u8)) != 0
}

#[script_bind]
//TODO: gamepad arg should be HidDeviceInstanceId
pub fn get_gamepad_axis(gamepad: i32, axis: &GamepadAxis) -> f64 {
    let Some(gamepad_state) = InputManager::instance().gamepad_states.get(&gamepad) else {
        println!("Client polled unknown gamepad ID {}", gamepad);
        return 0.0;
    };

    gamepad_state.axis_state[axis]
}

#[script_bind]
//TODO: gamepad arg should be HidDeviceInstanceId
pub fn get_gamepad_axis_delta(gamepad: i32, axis: GamepadAxis) -> f64 {
    let Some(gamepad_state) = InputManager::instance().gamepad_states.get(&gamepad) else {
        println!("Client polled unknown gamepad ID {}", gamepad);
        return 0.0;
    };

    gamepad_state.axis_state[&axis]
}

fn init_gamepads(devices_state: &mut GamepadDevicesState) {
    for joystick in SdlJoystick::get_all_joysticks().expect("Failed to enumerate joysticks") {
        let name = joystick.get_name().expect("Failed to get joystick name");
        if joystick.is_game_controller() {
            println!("Opening joystick '{}' as a gamepad", name);
            SdlGameController::from_instance_id(joystick.get_instance_id())
                .expect("Failed to open joystick as game controller");
            devices_state.available_gamepads.push(joystick.get_instance_id());
        } else {
            println!("Joystick '{}' is not reported as a gamepad, ignoring", name);
        }
    }

    let gamepad_count = devices_state.available_gamepads.len();
    if gamepad_count > 0 {
        println!(
            "{} connected gamepad{} found",
            gamepad_count,
            if gamepad_count != 1 { "s" } else { "" }
        );
    } else {
        println!("No gamepads connected");
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
    let Ok(gamepad) = SdlGameController::from_instance_id(id) else {
        println!("Failed to get SDL controller from instance ID {}", id);
        return;
    };

    let controller_opt = devices_state.mapped_gamepads.get(&id)
        .map(|name| InputManager::instance().get_controller(name));

    let mut new_button_state: GamepadButtonState = 0;
    for i in 0..SdlGameControllerButton::MaxValue.into() {
        let ordinal = SdlGameControllerButton::try_from_primitive(i);
        let bit = if gamepad.is_button_pressed(ordinal.unwrap()) { 1 } else { 0 };
        new_button_state |= bit << i;
    }

    let mut new_axis_state = HashMap::new();

    for (axis_1, axis_2) in AXIS_PAIRS {
        let axis_1_val = normalize_axis(gamepad.get_axis_value(AXIS_MAP_ARGUS_TO_SDL[&axis_1]));
        let axis_2_val = normalize_axis(gamepad.get_axis_value(AXIS_MAP_ARGUS_TO_SDL[&axis_2]));

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
                println!("Ignoring unhandled deadzone shape {:?}", shape);
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
    for event in sdl_get_events(
        SdlEventType::ControllerAxisMotion,
        SdlEventType::ControllerDeviceRemoved
    ) {
        match event.data {
            SdlEventData::ControllerButton(data) => {
                let Some(sdl_button) = BUTTON_MAP_SDL_TO_ARGUS.get(&data.button) else {
                    println!("Ignoring event for unknown gamepad button {:?}", data.button);
                    continue;
                };
                dispatch_button_events(sdl_button, data.state == SdlButtonState::Released);
            }
            SdlEventData::ControllerAxis(data) => {
                let Some(sdl_axis) = AXIS_MAP_SDL_TO_ARGUS.get(&data.axis) else {
                    println!("Ignoring event for unknown gamepad axis {:?}", data.axis);
                    continue;
                };
                //TODO: figure out what to do about the delta
                dispatch_axis_events(sdl_axis, normalize_axis(data.value), 0.0);
            }
            SdlEventData::ControllerDevice(data) => {
                match event.ty {
                    SdlEventType::ControllerDeviceAdded => {
                        let device_index = data.which;

                        let gamepad = SdlGameController::open(device_index).unwrap();

                        let Ok(instance_id) = gamepad.get_instance_id() else {
                            println!("Failed to get device instance ID of newly connected gamepad");
                            continue;
                        };

                        if devices_state.mapped_gamepads.contains_key(&instance_id) ||
                            devices_state.available_gamepads.contains(&instance_id) {
                            println!("Ignoring connect event for previously opened gamepad
                                      with instance ID {}", instance_id);
                            // this just decrements the ref count
                            gamepad.close();
                            continue;
                        }

                        devices_state.available_gamepads.push(instance_id);

                        let name = gamepad.get_name().unwrap();
                        println!(
                            "Gamepad '{}' with instance ID {} was connected",
                            name,
                            instance_id
                        );

                        dispatch_gamepad_connect_event(instance_id);
                    }
                    SdlEventType::ControllerDeviceRemoved => {
                        let instance_id = data.which as HidDeviceInstanceId;

                        let mut devices_state =
                            InputManager::instance().gamepad_devices_state.write();
                        if let Some(controller_name) =
                            devices_state.mapped_gamepads.get(&instance_id) {
                            match InputManager::instance().controllers.get_mut(controller_name) {
                                Some(mut controller) => {
                                    println!(
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

                            println!("Gamepad with instance ID {} was disconnected", instance_id);

                            dispatch_gamepad_disconnect_event("", instance_id);
                        }
                    }
                    _ => { continue; }
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
        println!("Client attempted to close unmapped gamepad instance ID {}", id);
        return;
    };

    devices_state.available_gamepads.push(id);
}

fn close_gamepad(id: HidDeviceInstanceId) {
    let Ok(controller) = SdlGameController::from_instance_id(id) else {
        println!("Failed to get SDL gamepad with instance ID {} while deinitializing gamepads", id);
        return;
    };
    controller.close();
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
