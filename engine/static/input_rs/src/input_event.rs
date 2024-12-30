use argus_scripting_bind::script_bind;
use core_rustabi::argus::core::{dispatch_event, register_event_handler, ArgusEvent, ArgusFfiEvent, Ordering, TargetThread};
use wm_rustabi::argus::wm::Window;
use crate::gamepad::HidDeviceInstanceId;

const EVENT_TYPE_INPUT: &str = "input";
const EVENT_TYPE_INPUT_DEVICE: &str = "input_device";

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
    fn get_type_id() -> &'static str {
        EVENT_TYPE_INPUT
    }
}

#[script_bind]
impl InputEvent {
    pub fn new(
        ty: InputEventType,
        window_id: impl Into<String>,
        controller_name: impl Into<String>,
        action: impl Into<String>,
        axis_value: f64,
        axis_delta: f64
    ) -> Self {
        Self {
            input_type: ty,
            window_id: window_id.into(),
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
    pub device_id: HidDeviceInstanceId,
}

impl InputDeviceEvent {
    pub fn new(
        ty: InputDeviceEventType,
        controller_name: impl Into<String>,
        device_id: HidDeviceInstanceId
    ) -> Self {
        Self {
            device_event: ty,
            controller_name: controller_name.into(),
            device_id,
        }
    }
}

impl ArgusEvent for InputDeviceEvent {
    fn get_type_id() -> &'static str{
        EVENT_TYPE_INPUT_DEVICE
    }
}

pub(crate) fn dispatch_button_event(
    window: Option<&Window>,
    controller_name: impl Into<String>,
    action: impl Into<String>,
    release: bool,
) {
    let event_type = if release { InputEventType::ButtonUp } else { InputEventType::ButtonDown };
    dispatch_event::<InputEvent>(InputEvent::new(
        event_type,
        window.map(|w| w.get_id()).unwrap_or_default(),
        controller_name.into(),
        action.into(),
        0.0,
        0.0,
    ));
}

pub(crate) fn dispatch_axis_event(
    window: Option<&Window>,
    controller_name: impl Into<String>,
    action: impl Into<String>,
    value: f64,
    delta: f64,
) {
    dispatch_event::<InputEvent>(InputEvent::new(
        InputEventType::AxisChanged,
        window.map(|w| w.get_id()).unwrap_or_default(),
        controller_name,
        action,
        value,
        delta,
    ));
}

#[script_bind]
pub fn register_input_handler(callback: fn(&InputEvent), ordering: Ordering) {
    register_event_handler(&callback, TargetThread::Update, ordering);
}
