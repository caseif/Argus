use std::collections::HashMap;
use bitmask::bitmask;
use lazy_static::lazy_static;
use wm_rustabi::argus::wm::{get_window_from_handle, Window};
use argus_scripting_bind::script_bind;
use sdl2::events::{sdl_get_events, SdlEventData, SdlEventType};
use sdl2::keyboard::{sdl_get_keyboard_state, SdlKeyCode, SdlScancode};
use sdl2::video::SdlWindow;
use crate::input_event::dispatch_button_event;
use crate::InputManager;

lazy_static! {
    static ref SCANCODE_MAP_ARGUS_TO_SDL:
    HashMap<KeyboardScancode, SdlScancode> = HashMap::from([
        (KeyboardScancode::Unknown, SdlScancode::Unknown),
        (KeyboardScancode::A, SdlScancode::A),
        (KeyboardScancode::B, SdlScancode::B),
        (KeyboardScancode::C, SdlScancode::C),
        (KeyboardScancode::D, SdlScancode::D),
        (KeyboardScancode::E, SdlScancode::E),
        (KeyboardScancode::F, SdlScancode::F),
        (KeyboardScancode::G, SdlScancode::G),
        (KeyboardScancode::H, SdlScancode::H),
        (KeyboardScancode::I, SdlScancode::I),
        (KeyboardScancode::J, SdlScancode::J),
        (KeyboardScancode::K, SdlScancode::K),
        (KeyboardScancode::L, SdlScancode::L),
        (KeyboardScancode::M, SdlScancode::M),
        (KeyboardScancode::N, SdlScancode::N),
        (KeyboardScancode::O, SdlScancode::O),
        (KeyboardScancode::P, SdlScancode::P),
        (KeyboardScancode::Q, SdlScancode::Q),
        (KeyboardScancode::R, SdlScancode::R),
        (KeyboardScancode::S, SdlScancode::S),
        (KeyboardScancode::T, SdlScancode::T),
        (KeyboardScancode::U, SdlScancode::U),
        (KeyboardScancode::V, SdlScancode::V),
        (KeyboardScancode::W, SdlScancode::W),
        (KeyboardScancode::X, SdlScancode::X),
        (KeyboardScancode::Y, SdlScancode::Y),
        (KeyboardScancode::Z, SdlScancode::Z),
        (KeyboardScancode::Number1, SdlScancode::Number1),
        (KeyboardScancode::Number2, SdlScancode::Number2),
        (KeyboardScancode::Number3, SdlScancode::Number3),
        (KeyboardScancode::Number4, SdlScancode::Number4),
        (KeyboardScancode::Number5, SdlScancode::Number5),
        (KeyboardScancode::Number6, SdlScancode::Number6),
        (KeyboardScancode::Number7, SdlScancode::Number7),
        (KeyboardScancode::Number8, SdlScancode::Number8),
        (KeyboardScancode::Number9, SdlScancode::Number9),
        (KeyboardScancode::Number0, SdlScancode::Number0),
        (KeyboardScancode::Enter, SdlScancode::Return),
        (KeyboardScancode::Escape, SdlScancode::Escape),
        (KeyboardScancode::Backspace, SdlScancode::Backspace),
        (KeyboardScancode::Tab, SdlScancode::Tab),
        (KeyboardScancode::Space, SdlScancode::Space),
        (KeyboardScancode::Minus, SdlScancode::Minus),
        (KeyboardScancode::Equals, SdlScancode::Equals),
        (KeyboardScancode::LeftBracket, SdlScancode::LeftBracket),
        (KeyboardScancode::RightBracket, SdlScancode::RightBracket),
        (KeyboardScancode::Backslash, SdlScancode::Backslash),
        (KeyboardScancode::Semicolon, SdlScancode::Semicolon),
        (KeyboardScancode::Apostrophe, SdlScancode::Apostrophe),
        (KeyboardScancode::Grave, SdlScancode::Grave),
        (KeyboardScancode::Comma, SdlScancode::Comma),
        (KeyboardScancode::Period, SdlScancode::Period),
        (KeyboardScancode::ForwardSlash, SdlScancode::Slash),
        (KeyboardScancode::CapsLock, SdlScancode::CapsLock),
        (KeyboardScancode::F1, SdlScancode::F1),
        (KeyboardScancode::F2, SdlScancode::F2),
        (KeyboardScancode::F3, SdlScancode::F3),
        (KeyboardScancode::F4, SdlScancode::F4),
        (KeyboardScancode::F6, SdlScancode::F6),
        (KeyboardScancode::F7, SdlScancode::F7),
        (KeyboardScancode::F8, SdlScancode::F8),
        (KeyboardScancode::F5, SdlScancode::F5),
        (KeyboardScancode::F9, SdlScancode::F9),
        (KeyboardScancode::F10, SdlScancode::F10),
        (KeyboardScancode::F11, SdlScancode::F11),
        (KeyboardScancode::F12, SdlScancode::F12),
        (KeyboardScancode::PrintScreen, SdlScancode::PrintScreen),
        (KeyboardScancode::ScrollLock, SdlScancode::ScrollLock),
        (KeyboardScancode::Pause, SdlScancode::Pause),
        (KeyboardScancode::Insert, SdlScancode::Insert),
        (KeyboardScancode::Home, SdlScancode::Home),
        (KeyboardScancode::PageUp, SdlScancode::PageUp),
        (KeyboardScancode::Delete, SdlScancode::Delete),
        (KeyboardScancode::End, SdlScancode::End),
        (KeyboardScancode::PageDown, SdlScancode::PageDown),
        (KeyboardScancode::ArrowRight, SdlScancode::Right),
        (KeyboardScancode::ArrowLeft, SdlScancode::Left),
        (KeyboardScancode::ArrowDown, SdlScancode::Down),
        (KeyboardScancode::ArrowUp, SdlScancode::Up),
        (KeyboardScancode::NumpadNumLock, SdlScancode::NumLockClear),
        (KeyboardScancode::NumpadDivide, SdlScancode::KpDivide),
        (KeyboardScancode::NumpadTimes, SdlScancode::KpMultiply),
        (KeyboardScancode::NumpadMinus, SdlScancode::KpMinus),
        (KeyboardScancode::NumpadPlus, SdlScancode::KpPlus),
        (KeyboardScancode::NumpadEnter, SdlScancode::KpEnter),
        (KeyboardScancode::Numpad1, SdlScancode::Kp1),
        (KeyboardScancode::Numpad2, SdlScancode::Kp2),
        (KeyboardScancode::Numpad3, SdlScancode::Kp3),
        (KeyboardScancode::Numpad4, SdlScancode::Kp4),
        (KeyboardScancode::Numpad5, SdlScancode::Kp5),
        (KeyboardScancode::Numpad6, SdlScancode::Kp6),
        (KeyboardScancode::Numpad7, SdlScancode::Kp7),
        (KeyboardScancode::Numpad8, SdlScancode::Kp8),
        (KeyboardScancode::Numpad9, SdlScancode::Kp9),
        (KeyboardScancode::Numpad0, SdlScancode::Kp0),
        (KeyboardScancode::NumpadDot, SdlScancode::KpPeriod),
        (KeyboardScancode::NumpadEquals, SdlScancode::KpEquals),
        (KeyboardScancode::Menu, SdlScancode::Menu),
        (KeyboardScancode::LeftControl, SdlScancode::LCtrl),
        (KeyboardScancode::LeftShift, SdlScancode::LShift),
        (KeyboardScancode::LeftAlt, SdlScancode::LAlt),
        (KeyboardScancode::Super, SdlScancode::LGui),
        (KeyboardScancode::RightControl, SdlScancode::RCtrl),
        (KeyboardScancode::RightShift, SdlScancode::RShift),
        (KeyboardScancode::RightAlt, SdlScancode::RAlt),
    ]);

    static ref SCANCODE_MAP_SDL_TO_ARGUS:
    HashMap<SdlScancode, KeyboardScancode> =
        SCANCODE_MAP_ARGUS_TO_SDL.iter().map(|(k, v)| (*v, *k)).collect();
}

/// Represents a scancode tied to a key press.
///
/// Argus's scancode definitions are based on a 104-key QWERTY layout.
///
/// Scancodes are indiciative of the location of a pressed key on the keyboard,
/// but the actual value of the key will depend on the current keyboard layout.
/// For instance, `KeyboardScancode::Q` will correspond to a press of the `A`
/// key if an AZERTY layout is active.
#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[script_bind]
pub enum KeyboardScancode {
    Unknown,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Number1,
    Number2,
    Number3,
    Number4,
    Number5,
    Number6,
    Number7,
    Number8,
    Number9,
    Number0,
    Enter,
    Escape,
    Backspace,
    Tab,
    Space,
    Minus,
    Equals,
    LeftBracket,
    RightBracket,
    Backslash,
    Semicolon,
    Apostrophe,
    Grave,
    Comma,
    Period,
    ForwardSlash,
    CapsLock,
    F1,
    F2,
    F3,
    F4,
    F6,
    F7,
    F8,
    F5,
    F9,
    F10,
    F11,
    F12,
    PrintScreen,
    ScrollLock,
    Pause,
    Insert,
    Home,
    PageUp,
    Delete,
    End,
    PageDown,
    ArrowRight,
    ArrowLeft,
    ArrowDown,
    ArrowUp,
    NumpadNumLock,
    NumpadDivide,
    NumpadTimes,
    NumpadMinus,
    NumpadPlus,
    NumpadEnter,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    Numpad0,
    NumpadDot,
    NumpadEquals,
    Menu,
    LeftControl,
    LeftShift,
    LeftAlt,
    Super,
    RightControl,
    RightShift,
    RightAlt,
}

/// Represents a command sent by a key press.
///
/// Command keys are defined as those which are not representative of a textual
/// character nor a key modifier.
#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[script_bind]
pub enum KeyboardCommand {
    Escape,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Backspace,
    Tab,
    CapsLock,
    Enter,
    Menu,
    PrintScreen,
    ScrollLock,
    Break,
    Insert,
    Home,
    PageUp,
    Delete,
    End,
    PageDown,
    ArrowUp,
    ArrowLeft,
    ArrowDown,
    ArrowRight,
    NumpadNumLock,
    NumpadEnter,
    NumpadDot,
    Super,
}

bitmask! {
    /// Represents a modifier enabled by a key press.
    ///
    /// Modifier keys are defined as the left and right shift, alt, and control
    /// keys, the Super key, and the num lock, caps lock, and scroll lock
    /// toggles.
    pub mask KeyboardModifiers : u16 where flags KeyboardModifier {
        None = 0x00,
        Shift = 0x01,
        Control = 0x02,
        Super = 0x04,
        Alt = 0x08
    }
}

impl KeyboardScancode {
    /// Gets the semantic name of the key associated with the given scancode.
    pub fn get_key_name(&self) -> String {
        todo!()
    }
}

impl KeyboardScancode {
    pub fn get_name(&self) -> String {
        SdlKeyCode::from_scancode(translate_argus_scancode(self)).get_name()
    }
}

pub(crate) fn translate_sdl_scancode(sdl_scancode: &SdlScancode) -> KeyboardScancode {
    match SCANCODE_MAP_SDL_TO_ARGUS.get(sdl_scancode) {
        Some(&argus_scancode) => argus_scancode,
        None => {
            println!("Received unknown keyboard scancode {:?}", sdl_scancode);
            KeyboardScancode::Unknown
        }
    }
}

pub(crate) fn translate_argus_scancode(argus_scancode: &KeyboardScancode) -> SdlScancode {
    match SCANCODE_MAP_ARGUS_TO_SDL.get(argus_scancode) {
        Some(&sdl_scancode) => sdl_scancode,
        None => {
            println!("Saw unknown Argus scancode {:?}", argus_scancode);
            SdlScancode::Unknown
        }
    }
}

pub(crate) fn init_keyboard(_window: &Window) {
}

/// Gets whether the key associated with a scancode is currently being pressed
/// down.
#[script_bind]
pub fn is_key_pressed(scancode: KeyboardScancode) -> bool {
    let kbd_state = InputManager::instance().keyboard_state.read();

    if kbd_state.key_states.is_empty() {
        return false;
    }

    let sdl_scancode = translate_argus_scancode(&scancode);
    let key_index = u32::from(sdl_scancode) as usize;

    if sdl_scancode == SdlScancode::Unknown {
        return false;
    }

    if key_index >= kbd_state.key_states.len() {
        return false;
    }

    kbd_state.key_states[key_index] != 0
}

//TODO: pretty sure this isn't needed - the returned array is updated by SDL_PumpEvents
fn poll_keyboard_state() {
    let mut kbd_state = InputManager::instance().keyboard_state.write();
    kbd_state.key_states = sdl_get_keyboard_state();
}

fn dispatch_events(window: &Window, key: KeyboardScancode, release: bool) {
    //TODO: ignore while in a TextInputContext once we properly implement that

    for item in &InputManager::instance().controllers {
        let controller_name = item.key();
        let controller = item.value();

        let Some(actions) = controller.get_keyboard_key_bindings(key) else { continue; };
        for action in actions {
            dispatch_button_event(Some(window), controller_name.clone(), action, release);
        }
    }
}

fn handle_keyboard_events() {
    let events = sdl_get_events(SdlEventType::KeyDown, SdlEventType::KeyUp);
    for event in events {
        let SdlEventData::Keyboard(data) = event.data else { continue; };
        if data.repeat != 0 {
            continue;
        }
        let sdl_window = SdlWindow::from_id(data.window_id).unwrap();
        let Some(window) = get_window_from_handle(sdl_window.get_handle()) else { return; };

        let key = translate_sdl_scancode(&data.keysym.scancode);
        dispatch_events(&window, key, event.ty == SdlEventType::KeyUp);
    }
}

pub(crate) fn update_keyboard() {
    poll_keyboard_state();
    handle_keyboard_events();
}
