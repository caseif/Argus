use std::ffi;
use std::mem::MaybeUninit;
use std::ops::Deref;
use num_enum::{IntoPrimitive, TryFromPrimitive, UnsafeFromPrimitive};
use crate::bindings::*;
use crate::gamecontroller::{SdlGameControllerAxis, SdlGameControllerButton};
use crate::internal::util::c_str_to_string_lossy;
use crate::joystick::SdlJoystickPowerLevel;
use crate::keyboard::{SdlKeyCode, SdlKeySym, SdlScancode};
use crate::mouse::SdlMouseButton;
use crate::wrapper::mouse::SdlMouseWheelDirection;

pub type SdlEventFilter = dyn Fn(&SdlEvent) -> i32;

#[allow(clippy::unnecessary_cast)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive,
    UnsafeFromPrimitive)]
#[repr(u32)]
pub enum SdlEventType {
    Unknown = SDL_FIRSTEVENT as u32,

    /* Application events */
    /// User-requested quit
    Quit = SDL_QUIT as u32,

    /* These application events have special meaning on iOS, see README-ios.md for details */
    /// The application is being terminated by the OS
    /// * Called on iOS in applicationWillTerminate()
    /// * Called on Android in onDestroy()
    AppTerminating = SDL_APP_TERMINATING as u32,
    /// The application is low on memory, free memory if possible.
    /// * Called on iOS in applicationDidReceiveMemoryWarning()
    /// * Called on Android in onLowMemory()
    AppLowMemory = SDL_APP_LOWMEMORY as u32,
    /// The application is about to enter the background
    // * Called on iOS in applicationWillResignActive()
    // * Called on Android in onPause()
    AppWillEnterBackground = SDL_APP_WILLENTERBACKGROUND as u32,
    /// The application did enter the background and may not get CPU for some time
    /// * Called on iOS in applicationDidEnterBackground()
    /// * Called on Android in onPause()
    AppDidEnterBackground = SDL_APP_DIDENTERBACKGROUND as u32,
    /// The application is about to enter the foreground
    /// * Called on iOS in applicationWillEnterForeground()
    /// * Called on Android in onResume()
    AppWillEnterForeground = SDL_APP_WILLENTERFOREGROUND as u32,
    /// The application is now interactive
    /// * Called on iOS in applicationDidBecomeActive()
    /// * Called on Android in onResume()
    AppDidEnterForeground = SDL_APP_DIDENTERFOREGROUND as u32,

    /// The user's locale preferences have changed.
    LocaleChanged = SDL_LOCALECHANGED as u32,

    /* Display events */
    /// Display state change
    DisplayEvent = SDL_DISPLAYEVENT as u32,

    /* Window events */
    /// Window state change
    WindowEvent = SDL_WINDOWEVENT as u32,
    /// System specific event
    SysWmEvent = SDL_SYSWMEVENT as u32,

    /* Keyboard events */
    /// Key pressed
    KeyDown = SDL_KEYDOWN as u32,
    /// Key released
    KeyUp = SDL_KEYUP as u32,
    /// Keyboard text editing (composition)
    TextEditing = SDL_TEXTEDITING as u32,
    /// Keyboard text input
    TextInput = SDL_TEXTINPUT as u32,
    /// Keymap changed due to a system event such as an
    /// input language or keyboard layout change.
    KeymapChanged = SDL_KEYMAPCHANGED as u32,
    /// Extended keyboard text editing (composition)
    TextEditingExt = SDL_TEXTEDITING_EXT as u32,

    /* Mouse events */
    /// Mouse moved
    MouseMotion = SDL_MOUSEMOTION as u32,
    /// Mouse button pressed
    MouseButtonDown = SDL_MOUSEBUTTONDOWN as u32,
    /// Mouse button released
    MouseButtonUp = SDL_MOUSEBUTTONUP as u32,
    /// Mouse wheel motion
    MouseWheel = SDL_MOUSEWHEEL as u32,

    /* Joystick events */
    /// Joystick axis motion
    JoyAxisMotion = SDL_JOYAXISMOTION as u32,
    /// Joystick trackball motion
    JoyBallMotion = SDL_JOYBALLMOTION as u32,
    /// Joystick hat position change
    JoyHatMotion = SDL_JOYHATMOTION as u32,
    /// Joystick button pressed
    JoyButtonDown = SDL_JOYBUTTONDOWN as u32,
    /// Joystick button released
    JoyButtonUp = SDL_JOYBUTTONUP as u32,
    /// A new joystick has been inserted into the system
    JoyDeviceAdded = SDL_JOYDEVICEADDED as u32,
    /// An opened joystick has been removed
    JoyDeviceRemoved = SDL_JOYDEVICEREMOVED as u32,
    /// Joystick battery level change
    JoyBatteryUpdated = SDL_JOYBATTERYUPDATED as u32,

    /* Game controller events */
    /// Game controller axis motion
    ControllerAxisMotion = SDL_CONTROLLERAXISMOTION as u32,
    /// Game controller button pressed
    ControllerButtonDown = SDL_CONTROLLERBUTTONDOWN as u32,
    /// Game controller button released
    ControllerButtonUp = SDL_CONTROLLERBUTTONUP as u32,
    /// A new Game controller has been inserted into the system
    ControllerDeviceAdded = SDL_CONTROLLERDEVICEADDED as u32,
    /// An opened Game controller has been removed
    ControllerDeviceRemoved = SDL_CONTROLLERDEVICEREMOVED as u32,
    /// The controller mapping was updated
    ControllerDeviceRemapped = SDL_CONTROLLERDEVICEREMAPPED as u32,
    /// Game controller touchpad was touched
    ControllerTouchpadDown = SDL_CONTROLLERTOUCHPADDOWN as u32,
    /// Game controller touchpad finger was moved
    ControllerTouchpadMotion = SDL_CONTROLLERTOUCHPADMOTION as u32,
    /// Game controller touchpad finger was lifted
    ControllerTouchpadUp = SDL_CONTROLLERTOUCHPADUP as u32,
    /// Game controller sensor was updated
    ControllerSensorUpdate = SDL_CONTROLLERSENSORUPDATE as u32,

    /* Touch events */
    FingerDown = SDL_FINGERDOWN as u32,
    FingerUp = SDL_FINGERUP as u32,
    FingerMotion = SDL_FINGERMOTION as u32,

    /* Gesture events */
    DollarGesture = SDL_DOLLARGESTURE as u32,
    DollarReccord = SDL_DOLLARRECORD as u32,
    MultiGesture = SDL_MULTIGESTURE as u32,

    /* Clipboard events */
    /// The clipboard or primary selection changed
    ClipboardUpdate = SDL_CLIPBOARDUPDATE as u32,

    /* Drag and drop events */
    /// The system requests a file open
    DropFile = SDL_DROPFILE as u32,
    /// text/plain drag-and-drop event
    DropText = SDL_DROPTEXT as u32,
    /// A new set of drops is beginning (NULL filename)
    DropBegin = SDL_DROPBEGIN as u32,
    /// Current set of drops is now complete (NULL filename)
    DropComplete = SDL_DROPCOMPLETE as u32,

    /* Audio hotplug events */
    /// A new audio device is available
    AudioDeviceAdded = SDL_AUDIODEVICEADDED as u32,
    /// An audio device has been removed.
    AudioDeviceRemoved = SDL_AUDIODEVICEREMOVED as u32,

    /* Sensor events */
    /// A sensor was updated
    SensorUpdate = SDL_SENSORUPDATE as u32,

    /* Render events */
    /// The render targets have been reset and their contents need to be updated
    RenderTargetsReset = SDL_RENDER_TARGETS_RESET as u32,
    /// The device has been reset and all textures need to be recreated
    RenderDeviceReset = SDL_RENDER_DEVICE_RESET as u32,

    /// Events `UserEvent` through `LastEvent` are for your use,
    /// and should be allocated with RegisterEvents()
    UserEvent = SDL_USEREVENT as u32,
    LastEvent = SDL_LASTEVENT as u32,
}

pub struct SdlEvent {
    pub ty: SdlEventType,
    /// In milliseconds, populated using SDL_GetTicks()
    pub timestamp: u32,
    /// Type-specific event data
    pub data: SdlEventData,
}

#[allow(clippy::unnecessary_cast)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive)]
#[repr(u8)]
pub enum SdlDisplayEventType {
    /// Never used
    None,
    /// Display orientation has changed to data1
    Orientation,
    /// Display has been added to the system
    Connected,
    /// Display has been removed from the system
    Disconnected,
    /// Display has changed position
    Moved,
}

#[allow(clippy::unnecessary_cast)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive)]
#[repr(u8)]
pub enum SdlWindowEventType {
    /// Never used
    None = SDL_WINDOWEVENT_NONE as u8,
    /// Window has been shown
    Shown = SDL_WINDOWEVENT_SHOWN as u8,
    /// Window has been hidden
    Hidden = SDL_WINDOWEVENT_HIDDEN as u8,
    /// Window has been exposed and should be redrawn
    Exposed = SDL_WINDOWEVENT_EXPOSED as u8,
    /// Window has been moved to data1, data2
    Moved = SDL_WINDOWEVENT_MOVED as u8,
    /// Window has been resized to data1xdata2
    Resized = SDL_WINDOWEVENT_RESIZED as u8,
    /// The window size has changed, either as a result of an API call or through the system or user
    /// changing the window size.
    SizeChanged = SDL_WINDOWEVENT_SIZE_CHANGED as u8,
    /// Window has been minimized
    Minimized = SDL_WINDOWEVENT_MINIMIZED as u8,
    /// Window has been maximized
    Maximized = SDL_WINDOWEVENT_MAXIMIZED as u8,
    /// Window has been restored to normal size and position
    Restored = SDL_WINDOWEVENT_RESTORED as u8,
    /// Window has gained mouse focus
    Enter = SDL_WINDOWEVENT_ENTER as u8,
    /// Window has lost mouse focus
    Leave = SDL_WINDOWEVENT_LEAVE as u8,
    /// Window has gained keyboard focus
    FocusGained = SDL_WINDOWEVENT_FOCUS_GAINED as u8,
    /// Window has lost keyboard focus
    FocusLost = SDL_WINDOWEVENT_FOCUS_LOST as u8,
    /// The window manager requests that the window be closed
    Close = SDL_WINDOWEVENT_CLOSE as u8,
    /// Window is being offered a focus (should SetWindowInputFocus() on itself or a subwindow, or
    /// ignore)
    TakeFocus = SDL_WINDOWEVENT_TAKE_FOCUS as u8,
    /// Window had a hit test that wasn't SDL_HITTEST_NORMAL.
    HitTest = SDL_WINDOWEVENT_HIT_TEST as u8,
    /// The ICC profile of the window's display has changed.
    IccProfileChanged = SDL_WINDOWEVENT_ICCPROF_CHANGED as u8,
    /// Window has been moved to display data1.
    DisplayChanged = SDL_WINDOWEVENT_DISPLAY_CHANGED as u8,
}

pub enum SdlEventData {
    Empty,
    Display(SdlDisplayEventData),
    Window(SdlWindowEventData),
    Keyboard(SdlKeyboardEventData),
    TextEditing(SdlTextEditingEventData),
    TextEditingExt(SdlTextEditingExtEventData),
    TextInput(SdlTextInputEventData),
    MouseMotion(SdlMouseMotionEventData),
    MouseButton(SdlMouseButtonEventData),
    MouseWheel(SdlMouseWheelEventData),
    JoyAxis(SdlJoyAxisEventData),
    JoyBall(SdlJoyBallEventData),
    JoyHat(SdlJoyHatEventData),
    JoyButton(SdlJoyButtonEventData),
    JoyDevice(SdlJoyDeviceEventData),
    JoyBattery(SdlJoyBatteryEventData),
    ControllerAxis(SdlControllerAxisEventData),
    ControllerButton(SdlControllerButtonEventData),
    ControllerDevice(SdlControllerDeviceEventData),
    ControllerTouchpad(SdlControllerTouchpadEventData),
    ControllerSensor(SdlControllerSensorEventData),
    AudioDevice(SdlAudioDeviceEventData),
    Sensor(SdlSensorEventData),
    Quit(SdlQuitEventData),
    User(SdlUserEventData),
    SysWm(SdlSysWmEventData),
    TouchFinger(SdlTouchFingerEventData),
    MultiGesture(SdlMultiGestureEventData),
    DollarGesture(SdlDollarGestureEventData),
    Drop(SdlDropEventData),
}

/// Display state change event
#[derive(Clone)]
pub struct SdlDisplayEventData {
    pub ty: SdlDisplayEventType,
    /// The associated display index
    pub display: u32,
    /// event dependent data
    pub data_1: i32,
}

/// Window state change event
#[derive(Clone)]
pub struct SdlWindowEventData {
    pub ty: SdlWindowEventType,
    /// The associated window
    pub window_id: u32,
    /// event dependent data
    pub data_1: i32,
    /// event dependent data
    pub data_2: i32,
}

/// Keyboard button event
#[derive(Clone)]
pub struct SdlKeyboardEventData {
    /// The associated window
    pub window_id: u32,
    /// `pub SdlKeyState::Pressed` or `pub SdlKeyState::Released`
    pub state: u8,
    /// Non-zero if this is a key repeat
    pub repeat: u8,
    /// The key that was pressed or released
    pub keysym: SdlKeySym,
}

/// Keyboard text editing event
#[derive(Clone)]
pub struct SdlTextEditingEventData {
    /// The window with keyboard focus, if any
    pub window_id: u32,
    /// The editing text
    pub text: String,
    /// The start cursor of selected editing text
    pub start: i32,
    /// The length of selected editing text
    pub length: i32,
}

/// Extended keyboard text editing event when text would be
/// truncated if stored in the text buffer SDL_TextEditingEvent
#[derive(Clone)]
pub struct SdlTextEditingExtEventData {
    /// The window with keyboard focus, if any
    pub window_id: u32,
    /// The editing text
    pub text: String,
    /// The start cursor of selected editing text
    pub start: i32,
    /// The length of selected editing text
    pub length: i32,
}

/// Keyboard text input event
#[derive(Clone)]
pub struct SdlTextInputEventData {
    /// The window with keyboard focus, if any
    pub window_id: u32,
    /// The input text
    pub text: String,
}

/// Mouse motion event
#[derive(Clone)]
pub struct SdlMouseMotionEventData {
    /// The window with mouse focus, if any
    pub window_id: u32,
    /// The mouse instance id, or SDL_TOUCH_MOUSEID
    pub which: u32,
    /// The current button state
    pub state: u32,
    /// X coordinate, relative to window
    pub x: i32,
    /// Y coordinate, relative to window
    pub y: i32,
    /// The relative motion in the X direction
    pub x_rel: i32,
    /// The relative motion in the Y direction
    pub y_rel: i32,
}

/// Mouse button event
#[derive(Clone)]
pub struct SdlMouseButtonEventData {
    /// The window with mouse focus, if any */
    pub window_id: u32,
    /// The mouse instance id, or SDL_TOUCH_MOUSEID */
    pub which: u32,
    /// The mouse button index */
    pub button: SdlMouseButton,
    /// ::SDL_PRESSED or ::SDL_RELEASED */
    pub state: SdlButtonState,
    /// 1 for single-click, 2 for double-click, etc. */
    pub clicks: u8,
    /// X coordinate, relative to window */
    pub x: i32,
    /// Y coordinate, relative to window */
    pub y: i32,
}

/// Mouse wheel event
#[derive(Clone)]
pub struct SdlMouseWheelEventData {
    /// The window with mouse focus, if any
    pub window_id: u32,
    /// The mouse instance id, or SDL_TOUCH_MOUSEID
    pub which: u32,
    /// The amount scrolled horizontally, positive to the right and negative to the left
    pub x: i32,
    /// The amount scrolled vertically, positive away from the user and negative toward the user
    pub y: i32,
    /// Set to one of the SDL_MOUSEWHEEL_* defines. When FLIPPED the values in X and Y will be
    /// opposite. Multiply by -1 to change them back
    pub direction: SdlMouseWheelDirection,
    /// The amount scrolled horizontally, positive to the right and negative to the left, with
    /// float precision (added in 2.0.18)
    pub precise_x: f32,
    /// The amount scrolled vertically, positive away from the user and negative toward the
    /// user, with float precision (added in 2.0.18)
    pub precise_y: f32,
    /// X coordinate, relative to window (added in 2.26.0)
    pub mouse_x: i32,
    /// Y coordinate, relative to window (added in 2.26.0)
    pub mouse_y: i32,
}

/// Joystick axis motion event
#[derive(Clone)]
pub struct SdlJoyAxisEventData {
    /// The joystick instance id
    pub which: i32,
    /// The joystick axis index
    pub axis: u8,
    /// The axis value (pub range: -32768 to 32767)
    pub value: i16,
}

/// Joystick trackball motion event
#[derive(Clone)]
pub struct SdlJoyBallEventData {
    /// The joystick instance id
    pub which: i32,
    /// The joystick trackball index
    pub ball: u8,
    /// The relative motion in the X direction
    pub xrel: i16,
    /// The relative motion in the Y direction
    pub yrel: i16,
}

/// Joystick hat position change event
#[derive(Clone)]
pub struct SdlJoyHatEventData {
    /// The joystick instance id
    pub which: i32,
    /// The joystick hat index
    pub hat: u8,
    /// The hat position value.
    ///
    /// Note that zero means the POV is centered.
    pub value: u8
}

/// Joystick button event
#[derive(Clone)]
pub struct SdlJoyButtonEventData {
    /// The joystick instance id
    pub which: i32,
    /// The joystick button index
    pub button: u8,
    /// ::SDL_PRESSED or ::SDL_RELEASED
    pub state: u8,
}

/// Joystick device event
#[derive(Clone)]
pub struct SdlJoyDeviceEventData {
    /// The joystick device index for the ADDED event, instance id for the REMOVED event
    pub which: i32,
}

/// Joysick battery level change event
#[derive(Clone)]
pub struct SdlJoyBatteryEventData {
    /// The joystick instance id
    pub which: i32,
    /// The joystick battery level
    pub level: SdlJoystickPowerLevel,
}

/// Game controller axis motion event
#[derive(Clone)]
pub struct SdlControllerAxisEventData {
    /// The joystick instance id
    pub which: i32,
    /// The controller axis (SDL_GameControllerAxis)
    pub axis: SdlGameControllerAxis,
    /// The axis value (pub range: -32768 to 32767)
    pub value: i16,
}

/// Game controller button event
#[derive(Clone)]
pub struct SdlControllerButtonEventData {
    /// The joystick instance id
    pub which: i32,
    /// The controller button (SdlGameControllerButton)
    pub button: SdlGameControllerButton,
    /// `pub SdlButtonState::Pressed` or `pub SdlButtonState::Released`
    pub state: SdlButtonState,
}

/// Controller device event
#[derive(Clone)]
pub struct SdlControllerDeviceEventData {
    /// The joystick device index for the ADDED event, instance id for the REMOVED or REMAPPED
    /// event
    pub which: i32,
}

/// Game controller touchpad event
#[derive(Clone)]
pub struct SdlControllerTouchpadEventData {
    /// The joystick instance id
    pub which: i32,
    /// The index of the touchpad
    pub touchpad: i32,
    /// The index of the finger on the touchpad
    pub finger: i32,
    /// Normalized in the range 0...1 with 0 being on the left
    pub x: f32,
    /// Normalized in the range 0...1 with 0 being at the top
    pub y: f32,
    /// Normalized in the range 0...1
    pub pressure: f32,
}

/// Game controller sensor event
#[derive(Clone)]
pub struct SdlControllerSensorEventData {
    /// The joystick instance id
    pub which: i32,
    /// The type of the sensor
    pub sensor: SdlSensorType,
    /// Up to 3 values from the sensor
    pub data: [f32; 3],
    /// The timestamp of the sensor reading in microseconds, if the hardware provides this
    /// information.
    pub timestamp_us: u64,
}

/// Audio device event
#[derive(Clone)]
pub struct SdlAudioDeviceEventData {
    /// The audio device index for the ADDED event (valid until next SDL_GetNumAudioDevices()
    /// call), SDL_AudioDeviceID for the REMOVED event
    pub which: u32,
    /// zero if an output device, non-zero if a capture device.
    pub is_capture: u8,
}

/// Sensor event
#[derive(Clone)]
pub struct SdlSensorEventData {
    /// The instance ID of the sensor
    pub which: i32,
    /// Up to 6 values from the sensor - additional values can be queried using
    /// SDL_SensorGetData()
    pub data: [f32; 6],
    /// The timestamp of the sensor reading in microseconds, if the hardware provides this
    /// information.
    pub timestamp_us: u64,
}

/// The "quit requested" event
#[derive(Clone)]
pub struct SdlQuitEventData {
}

/// A user-defined event type
#[derive(Clone)]
pub struct SdlUserEventData {
    pub ty: u32,
    /// The associated window if any
    pub window_id: u32,
    /// User defined event code
    pub code: i32,
}

/// A video driver dependent system event.
/// This event is disabled by default, you can enable it with SDL_EventState()
#[derive(Clone)]
pub struct SdlSysWmEventData {
}

/// Touch finger event
#[derive(Clone)]
pub struct SdlTouchFingerEventData {
    /// The window underneath the finger, if any
    pub window_id: u32,
    /// The touch device id
    pub touch_id: i64,
    pub finger_id: i64,
    /// Normalized in the range 0...1
    pub x: f32,
    /// Normalized in the range 0...1
    pub y: f32,
    /// Normalized in the range -1...1
    pub dx: f32,
    /// Normalized in the range -1...1
    pub dy: f32,
    /// Normalized in the range 0...1
    pub pressure: f32,
}

/// Multiple Finger Gesture Event
#[derive(Clone)]
pub struct SdlMultiGestureEventData {
    /// The touch device id
    pub touch_id: i64,
    pub d_theta: f32,
    pub d_dist: f32,
    pub x: f32,
    pub y: f32,
    pub num_fingers: u16,
    pub padding: u16,
}

/// Dollar Gesture Event
#[derive(Clone)]
pub struct SdlDollarGestureEventData {
    /// The touch device id
    pub touch_id: i64,
    pub gesture_id: i64,
    pub num_fingers: u32,
    pub error: f32,
    /// Normalized center of gesture
    pub x: f32,
    /// Normalized center of gesture
    pub y: f32,
}

/// An event used to request a file open by the system.
/// This event is enabled by default, you can disable it with SDL_EventState().
///
/// pub Note: If this event is enabled, you must free the filename in the event.
#[derive(Clone)]
pub struct SdlDropEventData {
    /// The window that was dropped on, if any
    pub window_id: u32,
    /// The file name, is `pub Option::None` on begin/complete
    pub file: Option<String>,
}

#[allow(clippy::unnecessary_cast)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive)]
#[repr(u8)]
pub enum SdlButtonState {
    Released = SDL_RELEASED as u8,
    Pressed = SDL_PRESSED as u8,
}

#[allow(clippy::unnecessary_cast)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive)]
#[repr(i32)]
pub enum SdlSensorType {
    /// Returned for an invalid sensor
    Invalid = SDL_SENSOR_INVALID,
    /// Unknown sensor type
    Unknown = SDL_SENSOR_UNKNOWN,
    /// Accelerometer
    Accel = SDL_SENSOR_ACCEL,
    /// Gyroscope
    Gyro = SDL_SENSOR_GYRO,
    /// Accelerometer for left Joy-Con controller and Wii nunchuk
    AccelL = SDL_SENSOR_ACCEL_L,
    /// Gyroscope for left Joy-Con controller
    GyroL = SDL_SENSOR_GYRO_L,
    /// Accelerometer for right Joy-Con controller
    AccelR = SDL_SENSOR_ACCEL_R,
    /// Gyroscope for right Joy-Con controller
    GyroR = SDL_SENSOR_GYRO_R,
}

fn convert_sdl_event(event: &SDL_Event) -> SdlEvent {
    unsafe {
        let data = match SdlEventType::unchecked_transmute_from(event.type_) {
            SdlEventType::Quit => SdlEventData::Quit(SdlQuitEventData {}),
            SdlEventType::DisplayEvent => SdlEventData::Display(SdlDisplayEventData {
                ty: SdlDisplayEventType::try_from_primitive(event.display.event).unwrap(),
                display: event.display.display,
                data_1: event.display.data1,
            }),
            SdlEventType::WindowEvent |
            SdlEventType::SysWmEvent => SdlEventData::Window(SdlWindowEventData {
                ty: SdlWindowEventType::try_from_primitive(event.window.event).unwrap(),
                window_id: event.window.windowID,
                data_1: event.window.data1,
                data_2: event.window.data2,
            }),
            SdlEventType::KeyDown |
            SdlEventType::KeyUp => SdlEventData::Keyboard(SdlKeyboardEventData {
                window_id: event.key.windowID,
                state: event.key.state,
                repeat: event.key.repeat,
                keysym: SdlKeySym {
                    scancode: SdlScancode::try_from_primitive(event.key.keysym.scancode as u32)
                        .unwrap(),
                    sym: SdlKeyCode::try_from_primitive(event.key.keysym.sym).unwrap(),
                    modifiers: event.key.keysym.mod_,
                },
            }),
            SdlEventType::TextEditing => SdlEventData::TextEditing(SdlTextEditingEventData {
                window_id: event.edit.windowID,
                text: c_str_to_string_lossy(event.edit.text.as_ptr()).unwrap(),
                start: event.edit.start,
                length: event.edit.length,
            }),
            SdlEventType::TextInput => SdlEventData::TextInput(SdlTextInputEventData {
                window_id: event.text.windowID,
                text: c_str_to_string_lossy(event.text.text.as_ptr()).unwrap(),
            }),
            SdlEventType::TextEditingExt => SdlEventData::TextEditingExt(SdlTextEditingExtEventData {
                window_id: event.editExt.windowID,
                text: c_str_to_string_lossy(event.editExt.text).unwrap(),
                start: event.editExt.start,
                length: event.editExt.length,
            }),
            SdlEventType::MouseMotion => SdlEventData::MouseMotion(SdlMouseMotionEventData {
                window_id: event.motion.windowID,
                which: event.motion.which,
                state: event.motion.state,
                x: event.motion.x,
                y: event.motion.y,
                x_rel: event.motion.xrel,
                y_rel: event.motion.yrel,
            }),
            SdlEventType::MouseButtonDown |
            SdlEventType::MouseButtonUp => SdlEventData::MouseButton(SdlMouseButtonEventData {
                window_id: event.button.windowID,
                which: event.button.which,
                button: SdlMouseButton::try_from_primitive(event.button.button).unwrap(),
                state: SdlButtonState::try_from_primitive(event.button.state).unwrap(),
                clicks: event.button.clicks,
                x: event.button.x,
                y: event.button.y,
            }),
            SdlEventType::MouseWheel => SdlEventData::MouseWheel(SdlMouseWheelEventData {
                window_id: event.wheel.windowID,
                which: event.wheel.which,
                x: event.wheel.x,
                y: event.wheel.y,
                direction: SdlMouseWheelDirection::try_from_primitive(event.wheel.direction).unwrap(),
                precise_x: event.wheel.preciseX,
                precise_y: event.wheel.preciseY,
                mouse_x: event.wheel.mouseX,
                mouse_y: event.wheel.mouseY,
            }),
            SdlEventType::JoyAxisMotion => SdlEventData::JoyAxis(SdlJoyAxisEventData {
                which: event.jaxis.which,
                axis: event.jaxis.axis,
                value: event.jaxis.value,
            }),
            SdlEventType::JoyBallMotion => SdlEventData::JoyBall(SdlJoyBallEventData {
                which: event.jball.which,
                ball: event.jball.ball,
                xrel: event.jball.xrel,
                yrel: event.jball.yrel,
            }),
            SdlEventType::JoyHatMotion => SdlEventData::JoyHat(SdlJoyHatEventData {
                which: event.jhat.which,
                hat: event.jhat.hat,
                value: event.jhat.value,
            }),
            SdlEventType::JoyButtonDown |
            SdlEventType::JoyButtonUp => SdlEventData::JoyButton(SdlJoyButtonEventData {
                which: event.jbutton.which,
                button: event.jbutton.button,
                state: event.jbutton.state,
            }),
            SdlEventType::JoyDeviceAdded |
            SdlEventType::JoyDeviceRemoved |
            SdlEventType::JoyBatteryUpdated => SdlEventData::JoyDevice(SdlJoyDeviceEventData {
                which: event.jdevice.which,
            }),
            SdlEventType::ControllerAxisMotion => SdlEventData::ControllerAxis(SdlControllerAxisEventData {
                which: event.caxis.which,
                axis: SdlGameControllerAxis::try_from_primitive(event.caxis.axis as i32).unwrap(),
                value: event.caxis.value,
            }),
            SdlEventType::ControllerButtonDown |
            SdlEventType::ControllerButtonUp => SdlEventData::ControllerButton(SdlControllerButtonEventData {
                which: event.cbutton.which,
                button: SdlGameControllerButton::try_from_primitive(event.cbutton.button as i32)
                    .unwrap(),
                state: SdlButtonState::try_from_primitive(event.cbutton.state).unwrap(),
            }),
            SdlEventType::ControllerDeviceAdded |
            SdlEventType::ControllerDeviceRemoved |
            SdlEventType::ControllerDeviceRemapped => SdlEventData::ControllerDevice(
                SdlControllerDeviceEventData {
                    which: event.cdevice.which,
                }
            ),
            SdlEventType::ControllerTouchpadDown |
            SdlEventType::ControllerTouchpadMotion |
            SdlEventType::ControllerTouchpadUp => SdlEventData::ControllerTouchpad(
                SdlControllerTouchpadEventData {
                    which: event.ctouchpad.which,
                    touchpad: event.ctouchpad.touchpad,
                    finger: event.ctouchpad.finger,
                    x: event.ctouchpad.x,
                    y: event.ctouchpad.y,
                    pressure: event.ctouchpad.pressure,
                }
            ),
            SdlEventType::ControllerSensorUpdate => SdlEventData::ControllerSensor(
                SdlControllerSensorEventData {
                    which: event.csensor.which,
                    sensor: SdlSensorType::try_from_primitive(event.csensor.sensor).unwrap(),
                    data: event.csensor.data,
                    timestamp_us: event.csensor.timestamp_us,
                }
            ),
            SdlEventType::FingerDown |
            SdlEventType::FingerUp |
            SdlEventType::FingerMotion => SdlEventData::TouchFinger(SdlTouchFingerEventData {
                window_id: event.tfinger.windowID,
                touch_id: event.tfinger.touchId,
                finger_id: event.tfinger.fingerId,
                x: event.tfinger.x,
                y: event.tfinger.y,
                dx: event.tfinger.dx,
                dy: event.tfinger.dy,
                pressure: event.tfinger.pressure,
            }),
            SdlEventType::DollarGesture => SdlEventData::DollarGesture(SdlDollarGestureEventData {
                touch_id: event.dgesture.touchId,
                gesture_id: event.dgesture.gestureId,
                num_fingers: event.dgesture.numFingers,
                error: event.dgesture.error,
                x: event.dgesture.x,
                y: event.dgesture.y,
            }),
            SdlEventType::MultiGesture => SdlEventData::MultiGesture(SdlMultiGestureEventData {
                touch_id: event.mgesture.touchId,
                d_theta: event.mgesture.dTheta,
                d_dist: event.mgesture.dDist,
                x: event.mgesture.x,
                y: event.mgesture.y,
                num_fingers: event.mgesture.numFingers,
                padding: event.mgesture.padding,
            }),
            SdlEventType::DropFile |
            SdlEventType::DropText |
            SdlEventType::DropBegin |
            SdlEventType::DropComplete => SdlEventData::Drop(SdlDropEventData {
                window_id: event.drop.windowID,
                file: c_str_to_string_lossy(event.drop.file),
            }),
            SdlEventType::AudioDeviceAdded |
            SdlEventType::AudioDeviceRemoved => SdlEventData::AudioDevice(SdlAudioDeviceEventData {
                which: event.adevice.which,
                is_capture: event.adevice.iscapture,
            }),
            SdlEventType::SensorUpdate => SdlEventData::Sensor(SdlSensorEventData {
                which: event.sensor.which,
                data: event.sensor.data,
                timestamp_us: event.sensor.timestamp_us,
            }),
            SdlEventType::UserEvent => SdlEventData::User(SdlUserEventData {
                ty: event.user.type_,
                window_id: event.user.windowID,
                code: event.user.code,
            }),
            _ => SdlEventData::Empty,
        };
        SdlEvent {
            ty: SdlEventType::try_from_primitive(event.common.type_)
                .unwrap_or(SdlEventType::Unknown),
            timestamp: event.common.timestamp,
            data,
        }
    }
}

fn peep_events(action: SDL_eventaction, min_type: SdlEventType, max_type: SdlEventType)
    -> Vec<SdlEvent> {
    unsafe {
        let mut event_vec = Vec::new();
        let mut buffer: MaybeUninit<[SDL_Event; 8]> = MaybeUninit::zeroed();
        loop {
            let count = SDL_PeepEvents(
                buffer.assume_init_mut().as_mut_ptr(),
                buffer.assume_init().len() as ffi::c_int,
                action,
                u32::from(min_type),
                u32::from(max_type),
            );
            if count == 0 {
                break;
            }
            event_vec.extend(
                buffer.assume_init()[0..count as usize]
                    .iter()
                    .map(convert_sdl_event)
            );
        }
        event_vec
    }
}

pub fn sdl_peek_events(min_type: SdlEventType, max_type: SdlEventType) -> Vec<SdlEvent> {
    peep_events(SDL_PEEKEVENT, min_type, max_type)
}

pub fn sdl_get_events(min_type: SdlEventType, max_type: SdlEventType) -> Vec<SdlEvent> {
    peep_events(SDL_GETEVENT, min_type, max_type)
}

extern "C" fn event_filter(
    userdata: *mut ffi::c_void,
    event: *mut SDL_Event
) -> ffi::c_int {
    let fn_ref = unsafe { (*(userdata as *mut Box<dyn Fn(&SdlEvent) -> i32>)).as_mut() };
    let retval = fn_ref(&convert_sdl_event(unsafe { &*event }));
    retval as ffi::c_int
}

pub fn sdl_add_event_watch<F: Fn(&SdlEvent) -> i32>(f: F) {
    let boxed_fn: Box<dyn Fn(&SdlEvent) -> i32> = Box::new(f);
    let dbl_boxed_fn = Box::new(boxed_fn);
    let userdata = Box::into_raw(dbl_boxed_fn) as *mut ffi::c_void;
    unsafe {
        SDL_AddEventWatch(
            Some(event_filter),
            userdata,
        );
    }
}

pub fn sdl_pump_events() {
    unsafe { SDL_PumpEvents() };
}
