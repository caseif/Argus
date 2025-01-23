use std::{ffi, ptr, slice};
use num_enum::{IntoPrimitive, TryFromPrimitive};
use crate::bindings::*;
use crate::error::c_str_to_string_or_error;

pub const SDL_NUM_SCANCODES: SDL_Scancode = 512;

#[allow(clippy::unnecessary_cast)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive)]
#[repr(u32)]
pub enum SdlScancode {
    Unknown = SDL_SCANCODE_UNKNOWN as u32,
    A = SDL_SCANCODE_A as u32,
    B = SDL_SCANCODE_B as u32,
    C = SDL_SCANCODE_C as u32,
    D = SDL_SCANCODE_D as u32,
    E = SDL_SCANCODE_E as u32,
    F = SDL_SCANCODE_F as u32,
    G = SDL_SCANCODE_G as u32,
    H = SDL_SCANCODE_H as u32,
    I = SDL_SCANCODE_I as u32,
    J = SDL_SCANCODE_J as u32,
    K = SDL_SCANCODE_K as u32,
    L = SDL_SCANCODE_L as u32,
    M = SDL_SCANCODE_M as u32,
    N = SDL_SCANCODE_N as u32,
    O = SDL_SCANCODE_O as u32,
    P = SDL_SCANCODE_P as u32,
    Q = SDL_SCANCODE_Q as u32,
    R = SDL_SCANCODE_R as u32,
    S = SDL_SCANCODE_S as u32,
    T = SDL_SCANCODE_T as u32,
    U = SDL_SCANCODE_U as u32,
    V = SDL_SCANCODE_V as u32,
    W = SDL_SCANCODE_W as u32,
    X = SDL_SCANCODE_X as u32,
    Y = SDL_SCANCODE_Y as u32,
    Z = SDL_SCANCODE_Z as u32,
    Number1 = SDL_SCANCODE_1 as u32,
    Number2 = SDL_SCANCODE_2 as u32,
    Number3 = SDL_SCANCODE_3 as u32,
    Number4 = SDL_SCANCODE_4 as u32,
    Number5 = SDL_SCANCODE_5 as u32,
    Number6 = SDL_SCANCODE_6 as u32,
    Number7 = SDL_SCANCODE_7 as u32,
    Number8 = SDL_SCANCODE_8 as u32,
    Number9 = SDL_SCANCODE_9 as u32,
    Number0 = SDL_SCANCODE_0 as u32,
    Return = SDL_SCANCODE_RETURN as u32,
    Escape = SDL_SCANCODE_ESCAPE as u32,
    Backspace = SDL_SCANCODE_BACKSPACE as u32,
    Tab = SDL_SCANCODE_TAB as u32,
    Space = SDL_SCANCODE_SPACE as u32,
    Minus = SDL_SCANCODE_MINUS as u32,
    Equals = SDL_SCANCODE_EQUALS as u32,
    LeftBracket = SDL_SCANCODE_LEFTBRACKET as u32,
    RightBracket = SDL_SCANCODE_RIGHTBRACKET as u32,
    /// Located at the lower left of the return key on ISO keyboards and at the right end of the
    /// QWERTY row on ANSI keyboards. Produces REVERSE SOLIDUS (backslash) and VERTICAL LINE in a US
    /// layout, REVERSE SOLIDUS and VERTICAL LINE in a UK Mac layout, NUMBER SIGN and TILDE in a UK
    /// Windows layout, DOLLAR SIGN and POUND SIGN in a Swiss German layout, NUMBER SIGN and
    /// APOSTROPHE in a German layout, GRAVE ACCENT and POUND SIGN in a French Mac layout, and
    /// ASTERISK and MICRO SIGN in a French Windows layout.
    Backslash = SDL_SCANCODE_BACKSLASH as u32,
    /// ISO USB keyboards actually use this code instead of 49 for the same key, but all OSes I've
    /// seen treat the two codes identically. So, as an implementor, unless your keyboard generates
    /// both of those codes and your OS treats them differently, you should generate SDL_SCANCODE
    /// BACKSLASH instead of this code. As a user, you should not rely on this code because SDL will
    /// never generate it with most (all?) keyboards.
    NonUsHash = SDL_SCANCODE_NONUSHASH as u32,
    Semicolon = SDL_SCANCODE_SEMICOLON as u32,
    Apostrophe = SDL_SCANCODE_APOSTROPHE as u32,
    /// Located in the top left corner (on both ANSI and ISO keyboards). Produces GRAVE ACCENT and
    /// TILDE in a US Windows layout and in US and UK Mac layouts on ANSI keyboards, GRAVE ACCENT
    /// and NOT SIGN in a UK Windows layout, SECTION SIGN and PLUS-MINUS SIGN in US and UK Mac
    /// layouts on ISO keyboards, SECTION SIGN and DEGREE SIGN in a Swiss German layout (Mac: only
    /// on ISO keyboards), CIRCUMFLEX ACCENT and DEGREE SIGN in a German layout (Mac: only on ISO
    /// keyboards), SUPERSCRIPT TWO and TILDE in a French Windows layout, COMMERCIAL AT and NUMBER
    /// SIGN in a French Mac layout on ISO keyboards, and LESS-THAN SIGN and GREATER-THAN SIGN in a
    /// Swiss German, German, or French Mac layout on ANSI keyboards.
    Grave = SDL_SCANCODE_GRAVE as u32,
    Comma = SDL_SCANCODE_COMMA as u32,
    Period = SDL_SCANCODE_PERIOD as u32,
    Slash = SDL_SCANCODE_SLASH as u32,
    CapsLock = SDL_SCANCODE_CAPSLOCK as u32,
    F1 = SDL_SCANCODE_F1 as u32,
    F2 = SDL_SCANCODE_F2 as u32,
    F3 = SDL_SCANCODE_F3 as u32,
    F4 = SDL_SCANCODE_F4 as u32,
    F5 = SDL_SCANCODE_F5 as u32,
    F6 = SDL_SCANCODE_F6 as u32,
    F7 = SDL_SCANCODE_F7 as u32,
    F8 = SDL_SCANCODE_F8 as u32,
    F9 = SDL_SCANCODE_F9 as u32,
    F10 = SDL_SCANCODE_F10 as u32,
    F11 = SDL_SCANCODE_F11 as u32,
    F12 = SDL_SCANCODE_F12 as u32,
    PrintScreen = SDL_SCANCODE_PRINTSCREEN as u32,
    ScrollLock = SDL_SCANCODE_SCROLLLOCK as u32,
    Pause = SDL_SCANCODE_PAUSE as u32,
    /// insert on PC, help on some Mac keyboards (but does send code 73, not 117)
    Insert = SDL_SCANCODE_INSERT as u32,
    Home = SDL_SCANCODE_HOME as u32,
    PageUp = SDL_SCANCODE_PAGEUP as u32,
    Delete = SDL_SCANCODE_DELETE as u32,
    End = SDL_SCANCODE_END as u32,
    PageDown = SDL_SCANCODE_PAGEDOWN as u32,
    Right = SDL_SCANCODE_RIGHT as u32,
    Left = SDL_SCANCODE_LEFT as u32,
    Down = SDL_SCANCODE_DOWN as u32,
    Up = SDL_SCANCODE_UP as u32,
    /// num lock on PC, clear on Mac keyboards
    NumLockClear = SDL_SCANCODE_NUMLOCKCLEAR as u32,
    KpDivide = SDL_SCANCODE_KP_DIVIDE as u32,
    KpMultiply = SDL_SCANCODE_KP_MULTIPLY as u32,
    KpMinus = SDL_SCANCODE_KP_MINUS as u32,
    KpPlus = SDL_SCANCODE_KP_PLUS as u32,
    KpEnter = SDL_SCANCODE_KP_ENTER as u32,
    Kp1 = SDL_SCANCODE_KP_1 as u32,
    Kp2 = SDL_SCANCODE_KP_2 as u32,
    Kp3 = SDL_SCANCODE_KP_3 as u32,
    Kp4 = SDL_SCANCODE_KP_4 as u32,
    Kp5 = SDL_SCANCODE_KP_5 as u32,
    Kp6 = SDL_SCANCODE_KP_6 as u32,
    Kp7 = SDL_SCANCODE_KP_7 as u32,
    Kp8 = SDL_SCANCODE_KP_8 as u32,
    Kp9 = SDL_SCANCODE_KP_9 as u32,
    Kp0 = SDL_SCANCODE_KP_0 as u32,
    KpPeriod = SDL_SCANCODE_KP_PERIOD as u32,
    /// This is the additional key that ISO keyboards have over ANSI ones, located between left
    /// shift and Y. Produces GRAVE ACCENT and TILDE in a US or UK Mac layout, REVERSE SOLIDUS
    /// (backslash) and VERTICAL LINE in a US or UK Windows layout, and LESS-THAN SIGN and
    /// GREATER-THAN SIGN in a Swiss German, German, or French layout.
    NonUsBackslash = SDL_SCANCODE_NONUSBACKSLASH as u32,
    /// windows contextual menu, compose
    Application = SDL_SCANCODE_APPLICATION as u32,
    /// The USB document says this is a status flag, not a physical key - but some Mac keyboards do
    /// have a power key.
    Power = SDL_SCANCODE_POWER as u32,
    KpEquals = SDL_SCANCODE_KP_EQUALS as u32,
    F13 = SDL_SCANCODE_F13 as u32,
    F14 = SDL_SCANCODE_F14 as u32,
    F15 = SDL_SCANCODE_F15 as u32,
    F16 = SDL_SCANCODE_F16 as u32,
    F17 = SDL_SCANCODE_F17 as u32,
    F18 = SDL_SCANCODE_F18 as u32,
    F19 = SDL_SCANCODE_F19 as u32,
    F20 = SDL_SCANCODE_F20 as u32,
    F21 = SDL_SCANCODE_F21 as u32,
    F22 = SDL_SCANCODE_F22 as u32,
    F23 = SDL_SCANCODE_F23 as u32,
    F24 = SDL_SCANCODE_F24 as u32,
    Execute = SDL_SCANCODE_EXECUTE as u32,
    /// AL Integrated Help Center
    Help = SDL_SCANCODE_HELP as u32,
    /// Menu (show menu)
    Menu = SDL_SCANCODE_MENU as u32,
    Select = SDL_SCANCODE_SELECT as u32,
    /// AC Stop
    Stop = SDL_SCANCODE_STOP as u32,
    /// AC Redo/Repeat
    Again = SDL_SCANCODE_AGAIN as u32,
    /// AC Undo
    Undo = SDL_SCANCODE_UNDO as u32,
    /// AC Cut
    Cut = SDL_SCANCODE_CUT as u32,
    /// AC Copy
    Copy = SDL_SCANCODE_COPY as u32,
    /// AC Paste
    Paste = SDL_SCANCODE_PASTE as u32,
    /// AC Find
    Find = SDL_SCANCODE_FIND as u32,
    Mute = SDL_SCANCODE_MUTE as u32,
    VolumeUp = SDL_SCANCODE_VOLUMEUP as u32,
    VolumeDown = SDL_SCANCODE_VOLUMEDOWN as u32,
    KpComma = SDL_SCANCODE_KP_COMMA as u32,
    KpEqualSas400 = SDL_SCANCODE_KP_EQUALSAS400 as u32,
    /// used on Asian keyboards, see footnotes in USB doc
    International1 = SDL_SCANCODE_INTERNATIONAL1 as u32,
    International2 = SDL_SCANCODE_INTERNATIONAL2 as u32,
    /// Yen
    International3 = SDL_SCANCODE_INTERNATIONAL3 as u32,
    International4 = SDL_SCANCODE_INTERNATIONAL4 as u32,
    International5 = SDL_SCANCODE_INTERNATIONAL5 as u32,
    International6 = SDL_SCANCODE_INTERNATIONAL6 as u32,
    International7 = SDL_SCANCODE_INTERNATIONAL7 as u32,
    International8 = SDL_SCANCODE_INTERNATIONAL8 as u32,
    International9 = SDL_SCANCODE_INTERNATIONAL9 as u32,
    /// Hangul/English toggle
    Lang1 = SDL_SCANCODE_LANG1 as u32,
    /// Hanja conversion
    Lang2 = SDL_SCANCODE_LANG2 as u32,
    /// Katakana
    Lang3 = SDL_SCANCODE_LANG3 as u32,
    /// Hiragana
    Lang4 = SDL_SCANCODE_LANG4 as u32,
    /// Zenkaku/Hankaku
    Lang5 = SDL_SCANCODE_LANG5 as u32,
    /// reserved
    Lang6 = SDL_SCANCODE_LANG6 as u32,
    /// reserved
    Lang7 = SDL_SCANCODE_LANG7 as u32,
    /// reserved
    Lang8 = SDL_SCANCODE_LANG8 as u32,
    /// reserved
    Lang9 = SDL_SCANCODE_LANG9 as u32,
    /// Erase-Eaze
    AltErase = SDL_SCANCODE_ALTERASE as u32,
    SysReq = SDL_SCANCODE_SYSREQ as u32,
    /// AC Cancel
    Cancel = SDL_SCANCODE_CANCEL as u32,
    Clear = SDL_SCANCODE_CLEAR as u32,
    Prior = SDL_SCANCODE_PRIOR as u32,
    Return2 = SDL_SCANCODE_RETURN2 as u32,
    Separator = SDL_SCANCODE_SEPARATOR as u32,
    Out = SDL_SCANCODE_OUT as u32,
    Oper = SDL_SCANCODE_OPER as u32,
    Clearagain = SDL_SCANCODE_CLEARAGAIN as u32,
    Crsel = SDL_SCANCODE_CRSEL as u32,
    Exsel = SDL_SCANCODE_EXSEL as u32,
    Kp00 = SDL_SCANCODE_KP_00 as u32,
    Kp000 = SDL_SCANCODE_KP_000 as u32,
    ThousandsSeparator = SDL_SCANCODE_THOUSANDSSEPARATOR as u32,
    DecimalSeparator = SDL_SCANCODE_DECIMALSEPARATOR as u32,
    CurrencyUnit = SDL_SCANCODE_CURRENCYUNIT as u32,
    CurrencySubunit = SDL_SCANCODE_CURRENCYSUBUNIT as u32,
    KpLeftParen = SDL_SCANCODE_KP_LEFTPAREN as u32,
    KpRightParen = SDL_SCANCODE_KP_RIGHTPAREN as u32,
    KpLeftBrace = SDL_SCANCODE_KP_LEFTBRACE as u32,
    KpRightBrace = SDL_SCANCODE_KP_RIGHTBRACE as u32,
    KpTab = SDL_SCANCODE_KP_TAB as u32,
    KpBackspace = SDL_SCANCODE_KP_BACKSPACE as u32,
    KpA = SDL_SCANCODE_KP_A as u32,
    KpB = SDL_SCANCODE_KP_B as u32,
    KpC = SDL_SCANCODE_KP_C as u32,
    KpD = SDL_SCANCODE_KP_D as u32,
    KpE = SDL_SCANCODE_KP_E as u32,
    KpF = SDL_SCANCODE_KP_F as u32,
    KpXor = SDL_SCANCODE_KP_XOR as u32,
    KpPower = SDL_SCANCODE_KP_POWER as u32,
    KpPercent = SDL_SCANCODE_KP_PERCENT as u32,
    KpLess = SDL_SCANCODE_KP_LESS as u32,
    KpGreater = SDL_SCANCODE_KP_GREATER as u32,
    KpAmpersand = SDL_SCANCODE_KP_AMPERSAND as u32,
    KpDblAmpersand = SDL_SCANCODE_KP_DBLAMPERSAND as u32,
    KpVerticalBar = SDL_SCANCODE_KP_VERTICALBAR as u32,
    KpDblVerticalBar = SDL_SCANCODE_KP_DBLVERTICALBAR as u32,
    KpColon = SDL_SCANCODE_KP_COLON as u32,
    KpHash = SDL_SCANCODE_KP_HASH as u32,
    KpSpace = SDL_SCANCODE_KP_SPACE as u32,
    KpAt = SDL_SCANCODE_KP_AT as u32,
    KpExclam = SDL_SCANCODE_KP_EXCLAM as u32,
    KpMemStore = SDL_SCANCODE_KP_MEMSTORE as u32,
    KpMemRecall = SDL_SCANCODE_KP_MEMRECALL as u32,
    KpMemClear = SDL_SCANCODE_KP_MEMCLEAR as u32,
    KpMemAdd = SDL_SCANCODE_KP_MEMADD as u32,
    KpMemSubtract = SDL_SCANCODE_KP_MEMSUBTRACT as u32,
    KpMemMultiply = SDL_SCANCODE_KP_MEMMULTIPLY as u32,
    KpMemDivide = SDL_SCANCODE_KP_MEMDIVIDE as u32,
    KpPlusMinus = SDL_SCANCODE_KP_PLUSMINUS as u32,
    KpClear = SDL_SCANCODE_KP_CLEAR as u32,
    KpClearEntry = SDL_SCANCODE_KP_CLEARENTRY as u32,
    KpBinary = SDL_SCANCODE_KP_BINARY as u32,
    KpOctal = SDL_SCANCODE_KP_OCTAL as u32,
    KpDecimal = SDL_SCANCODE_KP_DECIMAL as u32,
    KpHexadecimal = SDL_SCANCODE_KP_HEXADECIMAL as u32,
    LCtrl = SDL_SCANCODE_LCTRL as u32,
    LShift = SDL_SCANCODE_LSHIFT as u32,
    /// alt, option
    LAlt = SDL_SCANCODE_LALT as u32,
    /// windows, command (apple), meta
    LGui = SDL_SCANCODE_LGUI as u32,
    RCtrl = SDL_SCANCODE_RCTRL as u32,
    RShift = SDL_SCANCODE_RSHIFT as u32,
    /// alt gr, option
    RAlt = SDL_SCANCODE_RALT as u32,
    /// windows, command (apple), meta
    RGui = SDL_SCANCODE_RGUI as u32,
    /// I'm not sure if this is really not covered by any of the above, but since there's a special
    /// KMOD_MODE for it I'm adding it here
    Mode = SDL_SCANCODE_MODE as u32,
    AudioNext = SDL_SCANCODE_AUDIONEXT as u32,
    AudioPrev = SDL_SCANCODE_AUDIOPREV as u32,
    AudioStop = SDL_SCANCODE_AUDIOSTOP as u32,
    AudioPlay = SDL_SCANCODE_AUDIOPLAY as u32,
    AudioMute = SDL_SCANCODE_AUDIOMUTE as u32,
    MEDIASELECT = SDL_SCANCODE_MEDIASELECT as u32,
    /// AL Internet Browser
    Www = SDL_SCANCODE_WWW as u32,
    Mail = SDL_SCANCODE_MAIL as u32,
    /// AL Calculator
    Calculator = SDL_SCANCODE_CALCULATOR as u32,
    Computer = SDL_SCANCODE_COMPUTER as u32,
    /// AC Search
    AcSearch = SDL_SCANCODE_AC_SEARCH as u32,
    /// AC Home
    AcHome = SDL_SCANCODE_AC_HOME as u32,
    /// AC Back
    AcBack = SDL_SCANCODE_AC_BACK as u32,
    /// AC Forward
    AcForward = SDL_SCANCODE_AC_FORWARD as u32,
    /// AC Stop
    AcStop = SDL_SCANCODE_AC_STOP as u32,
    /// AC Refresh
    AcRefresh = SDL_SCANCODE_AC_REFRESH as u32,
    /// AC Bookmarks
    AcBookmarks = SDL_SCANCODE_AC_BOOKMARKS as u32,
    BrightnessDown = SDL_SCANCODE_BRIGHTNESSDOWN as u32,
    BrightnessUp = SDL_SCANCODE_BRIGHTNESSUP as u32,
    /// display mirroring/dual display switch, video mode switch
    DisplaySwitch = SDL_SCANCODE_DISPLAYSWITCH as u32,
    KbdIllumToggle = SDL_SCANCODE_KBDILLUMTOGGLE as u32,
    KbdIllumDown = SDL_SCANCODE_KBDILLUMDOWN as u32,
    KbdIllumUp = SDL_SCANCODE_KBDILLUMUP as u32,
    Eject = SDL_SCANCODE_EJECT as u32,
    /// SC System Sleep
    Sleep = SDL_SCANCODE_SLEEP as u32,
    App1 = SDL_SCANCODE_APP1 as u32,
    App2 = SDL_SCANCODE_APP2 as u32,
    AudioRewind = SDL_SCANCODE_AUDIOREWIND as u32,
    AudioFastForward = SDL_SCANCODE_AUDIOFASTFORWARD as u32,
    /// Usually situated below the display on phones and used as a multi-function feature key for
    /// selecting a software defined function shown on the bottom left of the display.
    SoftLeft = SDL_SCANCODE_SOFTLEFT as u32,
    /// Usually situated below the display on phones and used as a multi-function feature key for
    /// selecting a software defined function shown on the bottom right of the display.
    SoftRight = SDL_SCANCODE_SOFTRIGHT as u32,
    /// Used for accepting phone calls.
    Call = SDL_SCANCODE_CALL as u32,
    /// Used for rejecting phone calls.
    EndCall = SDL_SCANCODE_ENDCALL as u32,
}

#[allow(clippy::unnecessary_cast)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive)]
#[repr(i32)]
pub enum SdlKeyCode {
    Unknown = SDLK_UNKNOWN as i32,
    Return = SDLK_RETURN as i32,
    Escape = SDLK_ESCAPE as i32,
    Backspace = SDLK_BACKSPACE as i32,
    Tab = SDLK_TAB as i32,
    Space = SDLK_SPACE as i32,
    Exclaim = SDLK_EXCLAIM as i32,
    QuoteDbl = SDLK_QUOTEDBL as i32,
    Hash = SDLK_HASH as i32,
    Percent = SDLK_PERCENT as i32,
    Dollar = SDLK_DOLLAR as i32,
    Ampersand = SDLK_AMPERSAND as i32,
    Quote = SDLK_QUOTE as i32,
    LeftParen = SDLK_LEFTPAREN as i32,
    RightParen = SDLK_RIGHTPAREN as i32,
    Asterisk = SDLK_ASTERISK as i32,
    Plus = SDLK_PLUS as i32,
    Comma = SDLK_COMMA as i32,
    Minus = SDLK_MINUS as i32,
    Period = SDLK_PERIOD as i32,
    Slash = SDLK_SLASH as i32,
    Number0 = SDLK_0 as i32,
    Number1 = SDLK_1 as i32,
    Number2 = SDLK_2 as i32,
    Number3 = SDLK_3 as i32,
    Number4 = SDLK_4 as i32,
    Number5 = SDLK_5 as i32,
    Number6 = SDLK_6 as i32,
    Number7 = SDLK_7 as i32,
    Number8 = SDLK_8 as i32,
    Number9 = SDLK_9 as i32,
    Colon = SDLK_COLON as i32,
    Semicolon = SDLK_SEMICOLON as i32,
    Less = SDLK_LESS as i32,
    Equals = SDLK_EQUALS as i32,
    Greater = SDLK_GREATER as i32,
    Question = SDLK_QUESTION as i32,
    At = SDLK_AT as i32,
    LeftBracket = SDLK_LEFTBRACKET as i32,
    BackSlash = SDLK_BACKSLASH as i32,
    RightBracket = SDLK_RIGHTBRACKET as i32,
    Caret = SDLK_CARET as i32,
    Underscore = SDLK_UNDERSCORE as i32,
    BackQuote = SDLK_BACKQUOTE as i32,
    A = SDLK_a as i32,
    B = SDLK_b as i32,
    C = SDLK_c as i32,
    D = SDLK_d as i32,
    E = SDLK_e as i32,
    F = SDLK_f as i32,
    G = SDLK_g as i32,
    H = SDLK_h as i32,
    I = SDLK_i as i32,
    J = SDLK_j as i32,
    K = SDLK_k as i32,
    L = SDLK_l as i32,
    M = SDLK_m as i32,
    N = SDLK_n as i32,
    O = SDLK_o as i32,
    P = SDLK_p as i32,
    Q = SDLK_q as i32,
    R = SDLK_r as i32,
    S = SDLK_s as i32,
    T = SDLK_t as i32,
    U = SDLK_u as i32,
    V = SDLK_v as i32,
    W = SDLK_w as i32,
    X = SDLK_x as i32,
    Y = SDLK_y as i32,
    Z = SDLK_z as i32,
    CapsLock = SDLK_CAPSLOCK as i32,
    F1 = SDLK_F1 as i32,
    F2 = SDLK_F2 as i32,
    F3 = SDLK_F3 as i32,
    F4 = SDLK_F4 as i32,
    F5 = SDLK_F5 as i32,
    F6 = SDLK_F6 as i32,
    F7 = SDLK_F7 as i32,
    F8 = SDLK_F8 as i32,
    F9 = SDLK_F9 as i32,
    F10 = SDLK_F10 as i32,
    F11 = SDLK_F11 as i32,
    F12 = SDLK_F12 as i32,
    PrintScreen = SDLK_PRINTSCREEN as i32,
    ScrollLock = SDLK_SCROLLLOCK as i32,
    Pause = SDLK_PAUSE as i32,
    Insert = SDLK_INSERT as i32,
    Home = SDLK_HOME as i32,
    Pageup = SDLK_PAGEUP as i32,
    Delete = SDLK_DELETE as i32,
    End = SDLK_END as i32,
    PageDown = SDLK_PAGEDOWN as i32,
    Right = SDLK_RIGHT as i32,
    Left = SDLK_LEFT as i32,
    Down = SDLK_DOWN as i32,
    Up = SDLK_UP as i32,
    NumLockClear = SDLK_NUMLOCKCLEAR as i32,
    KpDivide = SDLK_KP_DIVIDE as i32,
    KpMultiply = SDLK_KP_MULTIPLY as i32,
    KpMinus = SDLK_KP_MINUS as i32,
    KpPlus = SDLK_KP_PLUS as i32,
    KpEnter = SDLK_KP_ENTER as i32,
    Kp1 = SDLK_KP_1 as i32,
    Kp2 = SDLK_KP_2 as i32,
    Kp3 = SDLK_KP_3 as i32,
    Kp4 = SDLK_KP_4 as i32,
    Kp5 = SDLK_KP_5 as i32,
    Kp6 = SDLK_KP_6 as i32,
    Kp7 = SDLK_KP_7 as i32,
    Kp8 = SDLK_KP_8 as i32,
    Kp9 = SDLK_KP_9 as i32,
    Kp0 = SDLK_KP_0 as i32,
    KpPeriod = SDLK_KP_PERIOD as i32,
    Application = SDLK_APPLICATION as i32,
    Power = SDLK_POWER as i32,
    KpEquals = SDLK_KP_EQUALS as i32,
    F13 = SDLK_F13 as i32,
    F14 = SDLK_F14 as i32,
    F15 = SDLK_F15 as i32,
    F16 = SDLK_F16 as i32,
    F17 = SDLK_F17 as i32,
    F18 = SDLK_F18 as i32,
    F19 = SDLK_F19 as i32,
    F20 = SDLK_F20 as i32,
    F21 = SDLK_F21 as i32,
    F22 = SDLK_F22 as i32,
    F23 = SDLK_F23 as i32,
    F24 = SDLK_F24 as i32,
    Execute = SDLK_EXECUTE as i32,
    Help = SDLK_HELP as i32,
    Menu = SDLK_MENU as i32,
    Select = SDLK_SELECT as i32,
    Stop = SDLK_STOP as i32,
    Again = SDLK_AGAIN as i32,
    Undo = SDLK_UNDO as i32,
    Cut = SDLK_CUT as i32,
    Copy = SDLK_COPY as i32,
    Paste = SDLK_PASTE as i32,
    Find = SDLK_FIND as i32,
    Mute = SDLK_MUTE as i32,
    VolumeUp = SDLK_VOLUMEUP as i32,
    VolumeDown = SDLK_VOLUMEDOWN as i32,
    KpComma = SDLK_KP_COMMA as i32,
    KpEqualsAs400 = SDLK_KP_EQUALSAS400 as i32,
    Alterase = SDLK_ALTERASE as i32,
    Sysreq = SDLK_SYSREQ as i32,
    Cancel = SDLK_CANCEL as i32,
    Clear = SDLK_CLEAR as i32,
    Prior = SDLK_PRIOR as i32,
    Return2 = SDLK_RETURN2 as i32,
    Separator = SDLK_SEPARATOR as i32,
    Out = SDLK_OUT as i32,
    Oper = SDLK_OPER as i32,
    ClearAgain = SDLK_CLEARAGAIN as i32,
    Crsel = SDLK_CRSEL as i32,
    Exsel = SDLK_EXSEL as i32,
    Kp00 = SDLK_KP_00 as i32,
    Kp000 = SDLK_KP_000 as i32,
    ThousandsSeparator = SDLK_THOUSANDSSEPARATOR as i32,
    DecimalSeparator = SDLK_DECIMALSEPARATOR as i32,
    CurrencyUnit = SDLK_CURRENCYUNIT as i32,
    CurrencySubunit = SDLK_CURRENCYSUBUNIT as i32,
    KpLeftParen = SDLK_KP_LEFTPAREN as i32,
    KpRightParen = SDLK_KP_RIGHTPAREN as i32,
    KpLeftBrace = SDLK_KP_LEFTBRACE as i32,
    KpRightBrace = SDLK_KP_RIGHTBRACE as i32,
    KpTab = SDLK_KP_TAB as i32,
    KpBackspace = SDLK_KP_BACKSPACE as i32,
    KpA = SDLK_KP_A as i32,
    KpB = SDLK_KP_B as i32,
    KpC = SDLK_KP_C as i32,
    KpD = SDLK_KP_D as i32,
    KpE = SDLK_KP_E as i32,
    KpF = SDLK_KP_F as i32,
    KpXor = SDLK_KP_XOR as i32,
    KpPower = SDLK_KP_POWER as i32,
    KpPercent = SDLK_KP_PERCENT as i32,
    KpLess = SDLK_KP_LESS as i32,
    KpGreater = SDLK_KP_GREATER as i32,
    KpAmpersand = SDLK_KP_AMPERSAND as i32,
    KpDblAmpersand = SDLK_KP_DBLAMPERSAND as i32,
    KpVerticalBar = SDLK_KP_VERTICALBAR as i32,
    KpDblVerticalBar = SDLK_KP_DBLVERTICALBAR as i32,
    KpColon = SDLK_KP_COLON as i32,
    KpHash = SDLK_KP_HASH as i32,
    KpSpace = SDLK_KP_SPACE as i32,
    KpAt = SDLK_KP_AT as i32,
    KpExclam = SDLK_KP_EXCLAM as i32,
    KpMemStore = SDLK_KP_MEMSTORE as i32,
    KpMemRecall = SDLK_KP_MEMRECALL as i32,
    KpMemClear = SDLK_KP_MEMCLEAR as i32,
    KpMemAdd = SDLK_KP_MEMADD as i32,
    KpMemSubtract = SDLK_KP_MEMSUBTRACT as i32,
    KpMemMultiply = SDLK_KP_MEMMULTIPLY as i32,
    KpMemDivide = SDLK_KP_MEMDIVIDE as i32,
    KpPlusMinus = SDLK_KP_PLUSMINUS as i32,
    KpClear = SDLK_KP_CLEAR as i32,
    KpClearEntry = SDLK_KP_CLEARENTRY as i32,
    KpBinary = SDLK_KP_BINARY as i32,
    KpOctal = SDLK_KP_OCTAL as i32,
    KpDecimal = SDLK_KP_DECIMAL as i32,
    KpHexadecimal = SDLK_KP_HEXADECIMAL as i32,
    LCtrl = SDLK_LCTRL as i32,
    LShift = SDLK_LSHIFT as i32,
    LAlt = SDLK_LALT as i32,
    LGui = SDLK_LGUI as i32,
    RCtrl = SDLK_RCTRL as i32,
    RShift = SDLK_RSHIFT as i32,
    RAlt = SDLK_RALT as i32,
    RGui = SDLK_RGUI as i32,
    Mode = SDLK_MODE as i32,
    AudioNext = SDLK_AUDIONEXT as i32,
    AudioPrev = SDLK_AUDIOPREV as i32,
    AudioStop = SDLK_AUDIOSTOP as i32,
    AudioPlay = SDLK_AUDIOPLAY as i32,
    AudioMute = SDLK_AUDIOMUTE as i32,
    MediaSelect = SDLK_MEDIASELECT as i32,
    Www = SDLK_WWW as i32,
    Mail = SDLK_MAIL as i32,
    Calculator = SDLK_CALCULATOR as i32,
    Computer = SDLK_COMPUTER as i32,
    AcSearch = SDLK_AC_SEARCH as i32,
    AcHome = SDLK_AC_HOME as i32,
    AcBack = SDLK_AC_BACK as i32,
    AcForward = SDLK_AC_FORWARD as i32,
    AcStop = SDLK_AC_STOP as i32,
    AcRefresh = SDLK_AC_REFRESH as i32,
    AcBookmarks = SDLK_AC_BOOKMARKS as i32,
    BrightnessDown = SDLK_BRIGHTNESSDOWN as i32,
    BrightnessUp = SDLK_BRIGHTNESSUP as i32,
    DisplaySwitch = SDLK_DISPLAYSWITCH as i32,
    KbdIllumToggle = SDLK_KBDILLUMTOGGLE as i32,
    KbdIllumDown = SDLK_KBDILLUMDOWN as i32,
    KbdIllumUp = SDLK_KBDILLUMUP as i32,
    Eject = SDLK_EJECT as i32,
    Sleep = SDLK_SLEEP as i32,
    App1 = SDLK_APP1 as i32,
    App2 = SDLK_APP2 as i32,
    AudioRewind = SDLK_AUDIOREWIND as i32,
    AudioFastForward = SDLK_AUDIOFASTFORWARD as i32,
    SoftLeft = SDLK_SOFTLEFT as i32,
    SoftRight = SDLK_SOFTRIGHT as i32,
    Call = SDLK_CALL as i32,
    EndCall = SDLK_ENDCALL as i32,
}

impl SdlKeyCode {
    pub fn from_scancode(scancode: SdlScancode) -> SdlKeyCode {
        unsafe {
            SdlKeyCode::try_from_primitive(SDL_GetKeyFromScancode((scancode as u32).into()))
                .unwrap()
        }
    }

    pub fn get_name(self) -> String {
        unsafe {
            #[allow(clippy::unwrap_or_default)]
            c_str_to_string_or_error(SDL_GetKeyName(self.into())).unwrap_or(String::new())
        }
    }
}

/// The SDL keysym structure, used in key events.
///
/// If you are looking for translated character input, see the `::SDL_TEXTINPUT` event.
#[derive(Clone)]
pub struct SdlKeySym {
    /// SDL physical key code - see `SdlScancode` for details
    pub scancode: SdlScancode,
    /// SDL virtual key code - see `SdlKeycode` for details
    pub sym: SdlKeyCode,
    /// current key modifiers
    pub modifiers: u16,
}

pub fn sdl_get_keyboard_state() -> &'static [u8] {
    let mut key_count: ffi::c_int = 0;
    // SAFETY: SDL state array is guaranteed to be valid for the lifetime of the program
    unsafe {
        let state_ptr = SDL_GetKeyboardState(ptr::addr_of_mut!(key_count));
        slice::from_raw_parts(state_ptr, key_count as usize)
    }
}
