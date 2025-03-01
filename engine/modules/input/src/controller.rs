/*
* This file is a part of Argus.
* Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
use std::collections::{HashMap, HashSet};
use std::hash::Hash;
use argus_logging::info;
use argus_scripting_bind::script_bind;
use crate::{DeadzoneConfig, InputManager, LOGGER};
use crate::gamepad::*;
use crate::keyboard::{is_key_pressed, KeyboardScancode};
use crate::mouse::*;

#[script_bind(ref_only)]
pub struct Controller {
    name: String,
    attached_gamepad: Option<i32>,
    pub(crate) was_gamepad_disconnected: bool,

    deadzones: DeadzoneConfig,
    bindings: ControllerBindings,
}

#[derive(Default)]
struct ControllerBindings {
    key_to_actions: HashMap<KeyboardScancode, HashSet<String>>,
    action_to_keys: HashMap<String, HashSet<KeyboardScancode>>,

    mouse_button_to_actions: HashMap<MouseButton, HashSet<String>>,
    action_to_mouse_buttons: HashMap<String, HashSet<MouseButton>>,

    mouse_axis_to_actions: HashMap<MouseAxis, HashSet<String>>,
    action_to_mouse_axes: HashMap<String, HashSet<MouseAxis>>,

    gamepad_button_to_actions: HashMap<GamepadButton, HashSet<String>>,
    action_to_gamepad_buttons: HashMap<String, HashSet<GamepadButton>>,

    gamepad_axis_to_actions: HashMap<GamepadAxis, HashSet<String>>,
    action_to_gamepad_axes: HashMap<String, HashSet<GamepadAxis>>,
}

fn max_abs(a: f64, b: f64) -> f64 {
    let a_sign = if a > 0.0 { 1.0 } else { -1.0 };
    let b_sign = if a > 0.0 { 1.0 } else { -1.0 };
    if a.abs() > b.abs() {
        a * a_sign
    } else {
        b * b_sign
    }
}

#[script_bind]
impl Controller {
    pub fn new(name: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            attached_gamepad: None,
            was_gamepad_disconnected: false,
            deadzones: Default::default(),
            bindings: Default::default(),
        }
    }

    #[script_bind]
    pub fn get_name(&self) -> &str {
        self.name.as_str()
    }

    #[script_bind]
    pub fn has_gamepad(&self) -> bool {
        self.attached_gamepad.is_some()
    }

    #[script_bind]
    //TODO: id should be HidDeviceInstanceId
    pub fn attach_gamepad(&mut self, id: i32) {
        if self.attached_gamepad.is_some() {
            panic!("Controller already has associated gamepad");
        }

        //TODO: figure out if we need to propagate this error
        assoc_gamepad(id, self.get_name()).expect("Failed to associate gamepad");
        self.attached_gamepad = Some(id);

        info!(
            LOGGER,
            "Attached gamepad '{}' to controller '{}'",
            self.get_gamepad_name(),
            self.get_name());
    }

    #[script_bind]
    pub fn attach_first_available_gamepad(&mut self) -> bool {
        if self.attached_gamepad.is_some() {
            panic!("Controller already has associated gamepad");
        }

        let Ok(id) = assoc_first_available_gamepad(self.get_name()) else { return false; };

        self.attached_gamepad = Some(id);

        info!(
            LOGGER,
            "Attached gamepad '{}' to controller '{}'",
            self.get_gamepad_name(),
            self.get_name()
        );

        true
    }

    #[script_bind]
    pub fn detach_gamepad(&mut self) {
        if let Some(gamepad) = self.attached_gamepad {
            unassoc_gamepad(gamepad);
            self.attached_gamepad = None;
        }
        // else silently fail
    }

    #[script_bind]
    pub fn get_gamepad_name(&self) -> String {
        match self.attached_gamepad {
            Some(gamepad) => get_gamepad_name(gamepad),
            None => panic!("Controller does not have an associated gamepad"),
        }
    }

    #[script_bind]
    pub fn get_deadzone_radius(&self) -> f64 {
        self.deadzones.radius.unwrap_or(InputManager::instance().get_global_deadzone_radius())
    }

    #[script_bind]
    pub fn set_deadzone_radius(&mut self, radius: f64) {
        self.deadzones.radius = Some(radius.clamp(0.0, 1.0));
    }

    #[script_bind]
    pub fn clear_deadzone_radius(&mut self) {
        self.deadzones.radius = None;
    }

    #[script_bind]
    pub fn get_deadzone_shape(&self) -> DeadzoneShape {
        self.deadzones.shape.unwrap_or(InputManager::instance().get_global_deadzone_shape())
    }

    #[script_bind]
    pub fn set_deadzone_shape(&mut self, shape: DeadzoneShape) {
        self.deadzones.shape = Some(shape);
    }

    #[script_bind]
    pub fn clear_deadzone_shape(&mut self) {
        self.deadzones.shape = None;
    }

    #[script_bind]
    pub fn get_axis_deadzone_radius(&self, axis: GamepadAxis) -> f64 {
        self.deadzones.axis_radii.get(&axis).cloned()
            .or_else(|| self.deadzones.radius)
            .unwrap_or_else(|| InputManager::instance().get_global_axis_deadzone_radius(&axis))
    }

    #[script_bind]
    pub fn set_axis_deadzone_radius(&mut self, axis: GamepadAxis, radius: f64) {
        self.deadzones.axis_radii.insert(axis, radius.clamp(0.0, 1.0));
    }

    #[script_bind]
    pub fn clear_axis_deadzone_radius(&mut self, axis: GamepadAxis) {
        self.deadzones.axis_radii.remove(&axis);
    }

    #[script_bind]
    pub fn get_axis_deadzone_shape(&self, axis: GamepadAxis) -> DeadzoneShape {
        self.deadzones.axis_shapes.get(&axis).cloned()
            .or_else(|| self.deadzones.shape)
            .unwrap_or_else(|| InputManager::instance().get_global_axis_deadzone_shape(&axis))
    }

    #[script_bind]
    pub fn set_axis_deadzone_shape(&mut self, axis: GamepadAxis, shape: DeadzoneShape) {
        self.deadzones.axis_shapes.insert(axis, shape);
    }

    pub fn clear_axis_deadzone_shape(&mut self, axis: GamepadAxis) {
        self.deadzones.axis_shapes.remove(&axis);
    }

    #[script_bind]
    pub fn unbind_action(&mut self, action: &str) {
        let Some(bound_keys) = self.bindings.action_to_keys.get(action) else { return; };

        // remove action from binding list of keys it's bound to
        for key in bound_keys {
            self.bindings.key_to_actions.remove(&key);
        }

        // remove binding list of action
        self.bindings.action_to_keys.remove(action);
    }

    pub fn get_keyboard_key_bindings(&self, key: KeyboardScancode) -> Option<&HashSet<String>> {
        self.bindings.key_to_actions.get(&key)
    }

    pub fn get_keyboard_action_bindings(&self, action: impl AsRef<str>)
        -> Option<&HashSet<KeyboardScancode>> {
        self.bindings.action_to_keys.get(action.as_ref())
    }

    #[script_bind]
    pub fn bind_keyboard_key(&mut self, key: KeyboardScancode, action: &str) {
        Self::bind_thing(
            &mut self.bindings.key_to_actions,
            &mut self.bindings.action_to_keys,
            key,
            action,
        );
    }

    #[script_bind]
    pub fn unbind_keyboard_key(&mut self, key: KeyboardScancode) {
        Self::unbind_thing(
            &mut self.bindings.key_to_actions,
            &mut self.bindings.action_to_keys,
            key,
            None,
        );
    }

    #[script_bind]
    pub fn unbind_keyboard_key_action(&mut self, key: KeyboardScancode, action: &str) {
        Self::unbind_thing(
            &mut self.bindings.key_to_actions,
            &mut self.bindings.action_to_keys,
            key,
            Some(action.as_ref()),
        );
    }

    #[script_bind]
    pub fn bind_mouse_button(&mut self, button: MouseButton, action: &str) {
        Self::bind_thing(
            &mut self.bindings.mouse_button_to_actions,
            &mut self.bindings.action_to_mouse_buttons,
            button,
            action,
        );
    }

    #[script_bind]
    pub fn unbind_mouse_button(&mut self, button: MouseButton) {
        Self::unbind_thing(
            &mut self.bindings.mouse_button_to_actions,
            &mut self.bindings.action_to_mouse_buttons,
            button,
            None,
        );
    }

    #[script_bind]
    pub fn unbind_mouse_button_action(&mut self, button: MouseButton, action: &str) {
        Self::unbind_thing(
            &mut self.bindings.mouse_button_to_actions,
            &mut self.bindings.action_to_mouse_buttons,
            button,
            Some(action),
        );
    }

    #[script_bind]
    pub fn bind_mouse_axis(&mut self, axis: MouseAxis, action: &str) {
        Self::bind_thing(
            &mut self.bindings.mouse_axis_to_actions,
            &mut self.bindings.action_to_mouse_axes,
            axis,
            action,
        );
    }

    #[script_bind]
    pub fn unbind_mouse_axis(&mut self, axis: MouseAxis) {
        Self::unbind_thing(
            &mut self.bindings.mouse_axis_to_actions,
            &mut self.bindings.action_to_mouse_axes,
            axis,
            None,
        );
    }

    #[script_bind]
    pub fn unbind_mouse_axis_action(&mut self, axis: MouseAxis, action: &str) {
        Self::unbind_thing(
            &mut self.bindings.mouse_axis_to_actions,
            &mut self.bindings.action_to_mouse_axes,
            axis,
            Some(action.as_ref()),
        );
    }

    #[script_bind]
    pub fn bind_gamepad_button(&mut self, button: GamepadButton, action: &str) {
        Self::bind_thing(
            &mut self.bindings.gamepad_button_to_actions,
            &mut self.bindings.action_to_gamepad_buttons,
            button,
            action,
        );
    }

    #[script_bind]
    pub fn unbind_gamepad_button(&mut self, button: GamepadButton) {
        Self::unbind_thing(
            &mut self.bindings.gamepad_button_to_actions,
            &mut self.bindings.action_to_gamepad_buttons,
            button,
            None,
        );
    }

    #[script_bind]
    pub fn unbind_gamepad_button_action(
        &mut self,
        button: GamepadButton,
        action: &str
    ) {
        Self::unbind_thing(
            &mut self.bindings.gamepad_button_to_actions,
            &mut self.bindings.action_to_gamepad_buttons,
            button,
            Some(action.as_ref()),
        );
    }

    #[script_bind]
    pub fn bind_gamepad_axis(&mut self, axis: GamepadAxis, action: &str) {
        Self::bind_thing(
            &mut self.bindings.gamepad_axis_to_actions,
            &mut self.bindings.action_to_gamepad_axes,
            axis,
            action,
        );
    }

    #[script_bind]
    pub fn unbind_gamepad_axis(&mut self, axis: GamepadAxis) {
        Self::unbind_thing(
            &mut self.bindings.gamepad_axis_to_actions,
            &mut self.bindings.action_to_gamepad_axes,
            axis,
            None,
        );
    }

    #[script_bind]
    pub fn unbind_gamepad_axis_action(&mut self, axis: GamepadAxis, action: &str) {
        Self::unbind_thing(
            &mut self.bindings.gamepad_axis_to_actions,
            &mut self.bindings.action_to_gamepad_axes,
            axis,
            Some(action.as_ref()),
        );
    }

    #[script_bind]
    pub fn is_gamepad_button_pressed(&self, button: GamepadButton) -> bool {
        let Some(gamepad) = self.attached_gamepad else {
            panic!("Cannot query gamepad button state for controller: No gamepad is associated");
        };

        is_gamepad_button_pressed(gamepad, button)
    }

    pub fn get_gamepad_axis(&self, axis: &GamepadAxis) -> f64 {
        let Some(gamepad) = self.attached_gamepad else {
            panic!("Cannot query gamepad axis state for controller: No gamepad is associated");
        };

        get_gamepad_axis(gamepad, axis)
    }

    #[script_bind]
    pub fn get_gamepad_axis_delta(&self, axis: GamepadAxis) -> f64 {
        let Some(gamepad) = self.attached_gamepad else {
            panic!("Cannot query gamepad axis state for controller: No gamepad is associated");
        };

        get_gamepad_axis_delta(gamepad, axis)
    }

    #[script_bind]
    pub fn is_action_pressed(&self, action: &str) -> bool {
        if let Some(key_bindings) = self.bindings.action_to_keys.get(action) {
            for key in key_bindings {
                if is_key_pressed(*key) {
                    return true;
                }
            }
        }

        if self.has_gamepad() {
            if let Some(gamepad_bindings) = self.bindings.action_to_gamepad_buttons
                .get(action) {
                for btn in gamepad_bindings {
                    if self.is_gamepad_button_pressed(*btn) {
                        return true;
                    }
                }
            }
        }

        if let Some(mouse_bindings) = self.bindings.action_to_mouse_buttons.get(action) {
            for btn in mouse_bindings {
                if is_mouse_button_pressed(*btn) {
                    return true;
                }
            }

        }

        false
    }

    #[script_bind]
    pub fn get_action_axis(&self, action: &str) -> f64 {
        let mut max_value: f64 = 0.0;
        if self.has_gamepad() {
            if let Some(gamepad_bindings) =
                self.bindings.action_to_gamepad_axes.get(action) {
                max_value = max_abs(
                    max_value,
                    gamepad_bindings.iter()
                        .map(|binding| self.get_gamepad_axis(binding))
                        .reduce(max_abs)
                        .unwrap_or(0.0),
                );
            }
        }

        if let Some(mouse_bindings) = self.bindings.action_to_mouse_axes.get(action) {
            max_value = max_abs(
                max_value,
                mouse_bindings.iter()
                    .map(|axis| get_mouse_axis(axis))
                    .reduce(max_abs)
                    .unwrap_or(0.0),
            );
        }

        max_value
    }

    #[script_bind]
    pub fn get_action_axis_delta(&self, action: &str) -> f64 {
        let mut max_value: f64 = 0.0;
        if self.has_gamepad() {
            if let Some(gamepad_bindings) =
                self.bindings.action_to_gamepad_axes.get(action) {
                max_value = max_abs(
                    max_value,
                    gamepad_bindings.iter()
                        .map(|axis| self.get_gamepad_axis_delta(*axis))
                        .reduce(max_abs)
                        .unwrap_or(0.0),
                );
            }
        }

        if let Some(mouse_bindings) = self.bindings.action_to_mouse_axes.get(action) {
            max_value = max_abs(
                max_value,
                mouse_bindings.iter()
                    .map(|axis| get_mouse_axis_delta (*axis))
                    .reduce(max_abs)
                    .unwrap_or(0.0),
            );
        }

        max_value
    }

    fn bind_thing<T: Copy + Eq + Hash>(
        to_map: &mut HashMap<T, HashSet<String>>,
        from_map: &mut HashMap<String, HashSet<T>>,
        thing: T,
        action: impl AsRef<str>
    ) {
        // we maintain two binding maps because actions and "things" have a
        // many-to-many relationship; i.e. each key may be bound to multiple
        // actions and each action may have multiple keys bound to it

        // insert into the thing-to-actions map
        to_map.entry(thing).or_default().insert(action.as_ref().to_string());

        // insert into the action-to-things map
        from_map.entry(action.as_ref().to_string()).or_default().insert(thing);
    }

    fn unbind_thing<T: Copy + Eq + Hash>(
        to_map: &mut HashMap<T, HashSet<String>>,
        from_map: &mut HashMap<String, HashSet<T>>,
        thing: T,
        action_opt: Option<&str>,
    ) {
        match action_opt {
            Some(action) => {
                if let Some(set) = from_map.get_mut(action) {
                    set.remove(&thing);
                }
                if let Some(v) = to_map.get_mut(&thing) {
                    v.remove(action);
                }
            }
            None => {
                for (_, bindings) in from_map {
                    bindings.remove(&thing);
                }
                to_map.remove(&thing);
            }
        }
    }

    pub(crate) fn get_mouse_button_actions(&self, button: &MouseButton)
        -> Option<&HashSet<String>> {
        self.bindings.mouse_button_to_actions.get(button)
    }

    pub(crate) fn get_mouse_axis_actions(&self, axis: &MouseAxis)
        -> Option<&HashSet<String>> {
        self.bindings.mouse_axis_to_actions.get(axis)
    }

    pub(crate) fn get_gamepad_button_actions(&self, button: &GamepadButton)
        -> Option<&HashSet<String>> {
        self.bindings.gamepad_button_to_actions.get(button)
    }

    pub(crate) fn get_gamepad_axis_actions(&self, axis: &GamepadAxis)
        -> Option<&HashSet<String>> {
        self.bindings.gamepad_axis_to_actions.get(axis)
    }
}

pub(crate) fn ack_gamepad_disconnects() {
    for mut item in InputManager::instance().controllers.iter_mut() {
        let controller = item.value_mut();
        if controller.was_gamepad_disconnected {
            // acknowledge disconnect flag set by render thread and fully
            // disassociate gamepad from controller
            controller.was_gamepad_disconnected = false;
            controller.detach_gamepad();
        }
    }
}
