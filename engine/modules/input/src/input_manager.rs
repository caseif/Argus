use std::collections::HashMap;
use std::ptr;
use dashmap::DashMap;
use dashmap::mapref::one::{Ref, RefMut};
use lazy_static::lazy_static;
use parking_lot::RwLock;
use argus_scripting_bind::script_bind;
use argus_util::math::Vector2d;
use crate::controller::Controller;
use crate::gamepad::{DeadzoneShape, GamepadAxis, HidDeviceInstanceId};

const MAX_CONTROLLERS: u32 = 8;
const DEF_DZ_RADIUS: f64 = 0.2;
const DEF_DZ_SHAPE: DeadzoneShape = DeadzoneShape::Ellipse;

lazy_static! {
    static ref INPUT_MANAGER_INSTANCE: InputManager = InputManager {
        controllers: DashMap::new(),
        keyboard_state: Default::default(),
        mouse_state: Default::default(),
        gamepad_devices_state: Default::default(),
        gamepad_states: DashMap::new(),
        deadzone_config: Default::default(),
    };
}

#[derive(Default)]
pub(crate) struct KeyboardState {
    pub(crate) key_states: &'static [u8],
}

#[derive(Default)]
pub(crate) struct MouseState {
    pub(crate) last_pos: Option<Vector2d>,
    pub(crate) delta: Vector2d,
    pub(crate) button_state: u32,
}

#[derive(Default)]
pub(crate) struct GamepadDevicesState {
    pub(crate) available_gamepads: Vec<HidDeviceInstanceId>,
    pub(crate) mapped_gamepads: HashMap<HidDeviceInstanceId, String>,
    pub(crate) are_gamepads_initted: bool,
}

#[derive(Default)]
pub(crate) struct GamepadState {
    pub(crate) button_state: u64,
    pub(crate) axis_state: HashMap<GamepadAxis, f64>,
    pub(crate) axis_deltas: HashMap<GamepadAxis, f64>,
}

pub struct DeadzoneConfig {
    pub(crate) radius: Option<f64>,
    pub(crate) shape: Option<DeadzoneShape>,
    pub(crate) axis_radii: HashMap<GamepadAxis, f64>,
    pub(crate) axis_shapes: HashMap<GamepadAxis, DeadzoneShape>,
}

impl Default for DeadzoneConfig {
    fn default() -> Self {
        Self {
            radius: Some(DEF_DZ_RADIUS),
            shape: Some(DEF_DZ_SHAPE),
            axis_radii: HashMap::new(),
            axis_shapes: HashMap::new(),
        }
    }
}

#[script_bind(ref_only)]
pub struct InputManager {
    pub(crate) controllers: DashMap<String, Controller>,
    pub(crate) keyboard_state: RwLock<KeyboardState>,
    pub(crate) mouse_state: RwLock<MouseState>,
    pub(crate) gamepad_devices_state: RwLock<GamepadDevicesState>,
    pub(crate) gamepad_states: DashMap<HidDeviceInstanceId, GamepadState>,
    pub(crate) deadzone_config: RwLock<DeadzoneConfig>,
}

#[script_bind]
pub fn get_input_manager() -> &'static InputManager {
    InputManager::instance()
}

#[script_bind]
impl InputManager {
    pub fn instance() -> &'static Self {
        &*INPUT_MANAGER_INSTANCE
    }

    pub fn get_controller(&self, name: impl AsRef<str>) -> Ref<String, Controller> {
        match self.controllers.get(name.as_ref()) {
            Some(controller) => controller,
            None => panic!("Invalid controller name: {}", name.as_ref()),
        }
    }

    pub fn get_controller_mut(&self, name: impl AsRef<str>) -> RefMut<String, Controller> {
        match self.controllers.get_mut(name.as_ref()) {
            Some(controller) => controller,
            None => panic!("Invalid controller name: {}", name.as_ref()),
        }
    }
    
    #[script_bind(rename = "get_controller")]
    pub fn get_controller_unsafe<'a>(&self, name: &str) -> &'a mut Controller {
        unsafe { &mut *ptr::from_mut(self.get_controller_mut(name).value_mut()) }
    }

    pub fn add_controller(&self, name: impl Into<String>) -> RefMut<String, Controller> {
        if self.controllers.len() >= MAX_CONTROLLERS as usize {
            panic!("Controller limit reached");
        }

        let name: String = name.into();
        let controller = Controller::new(name.clone());

        self.controllers.entry(name).or_insert_with(|| controller)
    }

    #[script_bind(rename = "add_controller")]
    pub fn add_controller_unsafe<'a>(&self, name: String) -> &'a mut Controller {
        unsafe { &mut *ptr::from_mut(self.add_controller(name).value_mut()) }
    }

    pub fn remove_controller(&self, name: impl AsRef<str>) {
        if self.controllers.remove(name.as_ref()).is_none() {
            panic!("Client attempted to remove unknown controller '{}'", name.as_ref());
        }
    }

    pub fn get_global_deadzone_radius(&self) -> f64 {
        self.deadzone_config.read().radius.unwrap()
    }

    pub fn set_global_deadzone_radius(&mut self, radius: f64) {
        self.deadzone_config.write().radius = Some(radius.clamp(0.0, 1.0));
    }

    pub fn get_global_deadzone_shape(&self) -> DeadzoneShape {
        self.deadzone_config.read().shape.unwrap()
    }

    pub fn set_global_deadzone_shape(&mut self, shape: DeadzoneShape) {
        self.deadzone_config.write().shape = Some(shape);
    }

    pub fn get_global_axis_deadzone_radius(&self, axis: &GamepadAxis) -> f64 {
        let deadzone_config = self.deadzone_config.read();
        deadzone_config.axis_radii.get(&axis)
            .cloned()
            .unwrap_or(deadzone_config.radius.unwrap())
    }

    pub fn set_global_axis_deadzone_radius(&mut self, axis: GamepadAxis, radius: f64) {
        self.deadzone_config.write().axis_radii.insert(axis, radius);
    }

    pub fn clear_global_axis_deadzone_radius(&mut self, axis: &GamepadAxis) {
        self.deadzone_config.write().axis_radii.remove(&axis);
    }

    pub fn get_global_axis_deadzone_shape(&self, axis: &GamepadAxis) -> DeadzoneShape {
        let deadzone_config = self.deadzone_config.read();
        deadzone_config.axis_shapes.get(&axis)
            .cloned()
            .unwrap_or(deadzone_config.shape.unwrap())
    }

    pub fn set_global_axis_deadzone_shape(&mut self, axis: GamepadAxis, shape: DeadzoneShape) {
        self.deadzone_config.write().axis_shapes.insert(axis, shape);
    }

    pub fn clear_global_axis_deadzone_shape(&mut self, axis: &GamepadAxis) {
        self.deadzone_config.write().axis_shapes.remove(&axis);
    }
}
