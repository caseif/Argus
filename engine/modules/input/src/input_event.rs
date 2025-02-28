use std::any::Any;
use argus_core::{dispatch_event, register_boxed_event_handler, ArgusEvent, Ordering, TargetThread};
use crate::gamepad::HidDeviceInstanceId;
use argus_scripting_bind::script_bind;

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[script_bind]
pub enum InputEventType {
    ButtonDown,
    ButtonUp,
    AxisChanged,
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[script_bind]
pub enum InputDeviceEventType {
    GamepadConnected,
    GamepadDisconnected,
}

#[script_bind(ref_only)]
pub struct InputEvent {
    pub input_type: InputEventType,
    //TODO: replace with Option once we have proper binding support for it
    pub window_id: String,
    pub controller_name: String,
    pub action: String,
    pub axis_value: f64,
    pub axis_delta: f64,
}

impl ArgusEvent for InputEvent {
    fn as_any_ref(&self) -> &dyn Any {
        self
    }
}

#[script_bind]
impl InputEvent {
    pub fn new(
        ty: InputEventType,
        window_id: Option<String>,
        controller_name: impl Into<String>,
        action: impl Into<String>,
        axis_value: f64,
        axis_delta: f64,
    ) -> Self {
        Self {
            input_type: ty,
            window_id: window_id.unwrap_or_default(),
            controller_name: controller_name.into(),
            action: action.into(),
            axis_value,
            axis_delta,
        }
    }

    #[script_bind]
    pub fn get_window_name(&self) -> &str {
        self.window_id.as_str()
    }
}

#[script_bind(ref_only)]
pub struct InputDeviceEvent {
    pub device_event: InputDeviceEventType,
    pub controller_name: String,
    //TODO: should be HidDeviceId, script_bind macro needs work to support this though
    pub device_id: i32,
}

impl InputDeviceEvent {
    pub fn new(
        ty: InputDeviceEventType,
        controller_name: impl Into<String>,
        device_id: HidDeviceInstanceId,
    ) -> Self {
        Self {
            device_event: ty,
            controller_name: controller_name.into(),
            device_id,
        }
    }
}

impl ArgusEvent for InputDeviceEvent {
    fn as_any_ref(&self) -> &dyn Any {
        self
    }
}

pub(crate) fn dispatch_button_event(
    window_id: Option<String>,
    controller_name: impl Into<String>,
    action: impl Into<String>,
    release: bool,
) {
    let event_type = if release {
        InputEventType::ButtonUp
    } else {
        InputEventType::ButtonDown
    };
    dispatch_event::<InputEvent>(InputEvent::new(
        event_type,
        window_id,
        controller_name.into(),
        action.into(),
        0.0,
        0.0,
    ));
}

pub(crate) fn dispatch_axis_event(
    window_id: Option<String>,
    controller_name: impl Into<String>,
    action: impl Into<String>,
    value: f64,
    delta: f64,
) {
    dispatch_event::<InputEvent>(InputEvent::new(
        InputEventType::AxisChanged,
        window_id,
        controller_name,
        action,
        value,
        delta,
    ));
}

#[script_bind]
pub fn register_input_handler(
    callback: Box<dyn 'static + Send + Fn(&InputEvent)>,
    ordering: Ordering
) {
    register_boxed_event_handler(callback, TargetThread::Update, ordering);
}

#[script_bind]
pub fn register_input_device_event_handler(
    callback: Box<dyn 'static + Send + Fn(&InputDeviceEvent)>,
    ordering: Ordering,
) {
    register_boxed_event_handler(callback, TargetThread::Update, ordering);
}
