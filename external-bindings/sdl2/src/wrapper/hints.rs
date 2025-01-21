use std::ffi::CString;
use std::str::FromStr;
use crate::bindings::SDL_SetHint;
use crate::internal::util::from_c_define;

pub const SDL_HINT_ACCELEROMETER_AS_JOYSTICK: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_ACCELEROMETER_AS_JOYSTICK) };

pub const SDL_HINT_ALLOW_ALT_TAB_WHILE_GRABBED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_ALLOW_ALT_TAB_WHILE_GRABBED) };

pub const SDL_HINT_ALLOW_TOPMOST: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_ALLOW_TOPMOST) };

pub const SDL_HINT_ANDROID_APK_EXPANSION_MAIN_FILE_VERSION: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_ANDROID_APK_EXPANSION_MAIN_FILE_VERSION) };

pub const SDL_HINT_ANDROID_APK_EXPANSION_PATCH_FILE_VERSION: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_ANDROID_APK_EXPANSION_PATCH_FILE_VERSION) };

pub const SDL_HINT_ANDROID_BLOCK_ON_PAUSE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_ANDROID_BLOCK_ON_PAUSE) };

pub const SDL_HINT_ANDROID_BLOCK_ON_PAUSE_PAUSEAUDIO: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_ANDROID_BLOCK_ON_PAUSE_PAUSEAUDIO) };

pub const SDL_HINT_ANDROID_TRAP_BACK_BUTTON: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_ANDROID_TRAP_BACK_BUTTON) };

pub const SDL_HINT_APP_NAME: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_APP_NAME) };

pub const SDL_HINT_APPLE_TV_CONTROLLER_UI_EVENTS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_APPLE_TV_CONTROLLER_UI_EVENTS) };

pub const SDL_HINT_APPLE_TV_REMOTE_ALLOW_ROTATION: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_APPLE_TV_REMOTE_ALLOW_ROTATION) };

pub const SDL_HINT_AUDIO_CATEGORY: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_AUDIO_CATEGORY) };

pub const SDL_HINT_AUDIO_DEVICE_APP_NAME: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_AUDIO_DEVICE_APP_NAME) };

pub const SDL_HINT_AUDIO_DEVICE_STREAM_NAME: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_AUDIO_DEVICE_STREAM_NAME) };

pub const SDL_HINT_AUDIO_DEVICE_STREAM_ROLE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_AUDIO_DEVICE_STREAM_ROLE) };

pub const SDL_HINT_AUDIO_RESAMPLING_MODE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_AUDIO_RESAMPLING_MODE) };

pub const SDL_HINT_AUTO_UPDATE_JOYSTICKS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_AUTO_UPDATE_JOYSTICKS) };

pub const SDL_HINT_AUTO_UPDATE_SENSORS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_AUTO_UPDATE_SENSORS) };

pub const SDL_HINT_BMP_SAVE_LEGACY_FORMAT: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_BMP_SAVE_LEGACY_FORMAT) };

pub const SDL_HINT_DISPLAY_USABLE_BOUNDS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_DISPLAY_USABLE_BOUNDS) };

pub const SDL_HINT_EMSCRIPTEN_ASYNCIFY: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_EMSCRIPTEN_ASYNCIFY) };

pub const SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT) };

pub const SDL_HINT_ENABLE_SCREEN_KEYBOARD: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_ENABLE_SCREEN_KEYBOARD) };

pub const SDL_HINT_ENABLE_STEAM_CONTROLLERS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_ENABLE_STEAM_CONTROLLERS) };

pub const SDL_HINT_EVENT_LOGGING: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_EVENT_LOGGING) };

pub const SDL_HINT_FORCE_RAISEWINDOW: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_FORCE_RAISEWINDOW) };

pub const SDL_HINT_FRAMEBUFFER_ACCELERATION: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_FRAMEBUFFER_ACCELERATION) };

pub const SDL_HINT_GAMECONTROLLERCONFIG: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_GAMECONTROLLERCONFIG) };

pub const SDL_HINT_GAMECONTROLLERCONFIG_FILE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_GAMECONTROLLERCONFIG_FILE) };

pub const SDL_HINT_GAMECONTROLLERTYPE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_GAMECONTROLLERTYPE) };

pub const SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES) };

pub const SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT) };

pub const SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS) };

pub const SDL_HINT_GRAB_KEYBOARD: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_GRAB_KEYBOARD) };

pub const SDL_HINT_HIDAPI_IGNORE_DEVICES: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_HIDAPI_IGNORE_DEVICES) };

pub const SDL_HINT_IDLE_TIMER_DISABLED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_IDLE_TIMER_DISABLED) };

pub const SDL_HINT_IME_INTERNAL_EDITING: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_IME_INTERNAL_EDITING) };

pub const SDL_HINT_IME_SHOW_UI: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_IME_SHOW_UI) };

pub const SDL_HINT_IME_SUPPORT_EXTENDED_TEXT: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_IME_SUPPORT_EXTENDED_TEXT) };

pub const SDL_HINT_IOS_HIDE_HOME_INDICATOR: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_IOS_HIDE_HOME_INDICATOR) };

pub const SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS) };

pub const SDL_HINT_JOYSTICK_HIDAPI: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI) };

pub const SDL_HINT_JOYSTICK_HIDAPI_GAMECUBE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_GAMECUBE) };

pub const SDL_HINT_JOYSTICK_GAMECUBE_RUMBLE_BRAKE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_GAMECUBE_RUMBLE_BRAKE) };

pub const SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS) };

pub const SDL_HINT_JOYSTICK_HIDAPI_COMBINE_JOY_CONS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_COMBINE_JOY_CONS) };

pub const SDL_HINT_JOYSTICK_HIDAPI_VERTICAL_JOY_CONS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_VERTICAL_JOY_CONS) };

pub const SDL_HINT_JOYSTICK_HIDAPI_LUNA: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_LUNA) };

pub const SDL_HINT_JOYSTICK_HIDAPI_NINTENDO_CLASSIC: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_NINTENDO_CLASSIC) };

pub const SDL_HINT_JOYSTICK_HIDAPI_SHIELD: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_SHIELD) };

pub const SDL_HINT_JOYSTICK_HIDAPI_PS3: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_PS3) };

pub const SDL_HINT_JOYSTICK_HIDAPI_PS4: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_PS4) };

pub const SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE) };

pub const SDL_HINT_JOYSTICK_HIDAPI_PS5: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_PS5) };

pub const SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED) };

pub const SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE) };

pub const SDL_HINT_JOYSTICK_HIDAPI_STADIA: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_STADIA) };

pub const SDL_HINT_JOYSTICK_HIDAPI_STEAM: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_STEAM) };

pub const SDL_HINT_JOYSTICK_HIDAPI_SWITCH: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_SWITCH) };

pub const SDL_HINT_JOYSTICK_HIDAPI_SWITCH_HOME_LED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_SWITCH_HOME_LED) };

pub const SDL_HINT_JOYSTICK_HIDAPI_JOYCON_HOME_LED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_JOYCON_HOME_LED) };

pub const SDL_HINT_JOYSTICK_HIDAPI_SWITCH_PLAYER_LED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_SWITCH_PLAYER_LED) };

pub const SDL_HINT_JOYSTICK_HIDAPI_WII: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_WII) };

pub const SDL_HINT_JOYSTICK_HIDAPI_WII_PLAYER_LED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_WII_PLAYER_LED) };

pub const SDL_HINT_JOYSTICK_HIDAPI_XBOX: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_XBOX) };

pub const SDL_HINT_JOYSTICK_HIDAPI_XBOX_360: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_XBOX_360) };

pub const SDL_HINT_JOYSTICK_HIDAPI_XBOX_360_PLAYER_LED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_XBOX_360_PLAYER_LED) };

pub const SDL_HINT_JOYSTICK_HIDAPI_XBOX_360_WIRELESS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_XBOX_360_WIRELESS) };

pub const SDL_HINT_JOYSTICK_HIDAPI_XBOX_ONE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_XBOX_ONE) };

pub const SDL_HINT_JOYSTICK_HIDAPI_XBOX_ONE_HOME_LED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_HIDAPI_XBOX_ONE_HOME_LED) };

pub const SDL_HINT_JOYSTICK_RAWINPUT: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_RAWINPUT) };

pub const SDL_HINT_JOYSTICK_RAWINPUT_CORRELATE_XINPUT: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_RAWINPUT_CORRELATE_XINPUT) };

pub const SDL_HINT_JOYSTICK_ROG_CHAKRAM: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_ROG_CHAKRAM) };

pub const SDL_HINT_JOYSTICK_THREAD: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_THREAD) };

pub const SDL_HINT_JOYSTICK_WGI: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_WGI) };

pub const SDL_HINT_KMSDRM_REQUIRE_DRM_MASTER: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_KMSDRM_REQUIRE_DRM_MASTER) };

pub const SDL_HINT_JOYSTICK_DEVICE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_JOYSTICK_DEVICE) };

pub const SDL_HINT_LINUX_DIGITAL_HATS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_LINUX_DIGITAL_HATS) };

pub const SDL_HINT_LINUX_HAT_DEADZONES: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_LINUX_HAT_DEADZONES) };

pub const SDL_HINT_LINUX_JOYSTICK_CLASSIC: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_LINUX_JOYSTICK_CLASSIC) };

pub const SDL_HINT_LINUX_JOYSTICK_DEADZONES: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_LINUX_JOYSTICK_DEADZONES) };

pub const SDL_HINT_MAC_BACKGROUND_APP: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MAC_BACKGROUND_APP) };

pub const SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK) };

pub const SDL_HINT_MAC_OPENGL_ASYNC_DISPATCH: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MAC_OPENGL_ASYNC_DISPATCH) };

pub const SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS) };

pub const SDL_HINT_MOUSE_DOUBLE_CLICK_TIME: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_DOUBLE_CLICK_TIME) };

pub const SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH) };

pub const SDL_HINT_MOUSE_NORMAL_SPEED_SCALE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_NORMAL_SPEED_SCALE) };

pub const SDL_HINT_MOUSE_RELATIVE_MODE_CENTER: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_RELATIVE_MODE_CENTER) };

pub const SDL_HINT_MOUSE_RELATIVE_MODE_WARP: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_RELATIVE_MODE_WARP) };

pub const SDL_HINT_MOUSE_RELATIVE_SCALING: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_RELATIVE_SCALING) };

pub const SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE) };

pub const SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE) };

pub const SDL_HINT_MOUSE_RELATIVE_WARP_MOTION: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_RELATIVE_WARP_MOTION) };

pub const SDL_HINT_MOUSE_TOUCH_EVENTS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_TOUCH_EVENTS) };

pub const SDL_HINT_MOUSE_AUTO_CAPTURE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_MOUSE_AUTO_CAPTURE) };

pub const SDL_HINT_NO_SIGNAL_HANDLERS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_NO_SIGNAL_HANDLERS) };

pub const SDL_HINT_OPENGL_ES_DRIVER: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_OPENGL_ES_DRIVER) };

pub const SDL_HINT_ORIENTATIONS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_ORIENTATIONS) };

pub const SDL_HINT_POLL_SENTINEL: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_POLL_SENTINEL) };

pub const SDL_HINT_PREFERRED_LOCALES: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_PREFERRED_LOCALES) };

pub const SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION) };

pub const SDL_HINT_QTWAYLAND_WINDOW_FLAGS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_QTWAYLAND_WINDOW_FLAGS) };

pub const SDL_HINT_RENDER_BATCHING: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RENDER_BATCHING) };

pub const SDL_HINT_RENDER_LINE_METHOD: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RENDER_LINE_METHOD) };

pub const SDL_HINT_RENDER_DIRECT3D11_DEBUG: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RENDER_DIRECT3D11_DEBUG) };

pub const SDL_HINT_RENDER_DIRECT3D_THREADSAFE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RENDER_DIRECT3D_THREADSAFE) };

pub const SDL_HINT_RENDER_DRIVER: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RENDER_DRIVER) };

pub const SDL_HINT_RENDER_LOGICAL_SIZE_MODE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RENDER_LOGICAL_SIZE_MODE) };

pub const SDL_HINT_RENDER_OPENGL_SHADERS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RENDER_OPENGL_SHADERS) };

pub const SDL_HINT_RENDER_SCALE_QUALITY: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RENDER_SCALE_QUALITY) };

pub const SDL_HINT_RENDER_VSYNC: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RENDER_VSYNC) };

pub const SDL_HINT_RENDER_METAL_PREFER_LOW_POWER_DEVICE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RENDER_METAL_PREFER_LOW_POWER_DEVICE) };

pub const SDL_HINT_PS2_DYNAMIC_VSYNC: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_PS2_DYNAMIC_VSYNC) };

pub const SDL_HINT_RETURN_KEY_HIDES_IME: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RETURN_KEY_HIDES_IME) };

pub const SDL_HINT_RPI_VIDEO_LAYER: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_RPI_VIDEO_LAYER) };

pub const SDL_HINT_SCREENSAVER_INHIBIT_ACTIVITY_NAME: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_SCREENSAVER_INHIBIT_ACTIVITY_NAME) };

pub const SDL_HINT_THREAD_FORCE_REALTIME_TIME_CRITICAL: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_THREAD_FORCE_REALTIME_TIME_CRITICAL) };

pub const SDL_HINT_THREAD_PRIORITY_POLICY: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_THREAD_PRIORITY_POLICY) };

pub const SDL_HINT_THREAD_STACK_SIZE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_THREAD_STACK_SIZE) };

pub const SDL_HINT_TIMER_RESOLUTION: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_TIMER_RESOLUTION) };

pub const SDL_HINT_TOUCH_MOUSE_EVENTS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_TOUCH_MOUSE_EVENTS) };

pub const SDL_HINT_VITA_TOUCH_MOUSE_DEVICE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VITA_TOUCH_MOUSE_DEVICE) };

pub const SDL_HINT_TV_REMOTE_AS_JOYSTICK: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_TV_REMOTE_AS_JOYSTICK) };

pub const SDL_HINT_VIDEO_ALLOW_SCREENSAVER: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_ALLOW_SCREENSAVER) };

pub const SDL_HINT_VIDEO_DOUBLE_BUFFER: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_DOUBLE_BUFFER) };

pub const SDL_HINT_VIDEO_EGL_ALLOW_TRANSPARENCY: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_EGL_ALLOW_TRANSPARENCY) };

pub const SDL_HINT_VIDEO_EXTERNAL_CONTEXT: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_EXTERNAL_CONTEXT) };

pub const SDL_HINT_VIDEO_HIGHDPI_DISABLED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_HIGHDPI_DISABLED) };

pub const SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES) };

pub const SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS) };

pub const SDL_HINT_VIDEO_WAYLAND_ALLOW_LIBDECOR: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_WAYLAND_ALLOW_LIBDECOR) };

pub const SDL_HINT_VIDEO_WAYLAND_PREFER_LIBDECOR: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_WAYLAND_PREFER_LIBDECOR) };

pub const SDL_HINT_VIDEO_WAYLAND_MODE_EMULATION: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_WAYLAND_MODE_EMULATION) };

pub const SDL_HINT_VIDEO_WAYLAND_EMULATE_MOUSE_WARP: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_WAYLAND_EMULATE_MOUSE_WARP) };

pub const SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT) };

pub const SDL_HINT_VIDEO_FOREIGN_WINDOW_OPENGL: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_FOREIGN_WINDOW_OPENGL) };

pub const SDL_HINT_VIDEO_FOREIGN_WINDOW_VULKAN: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_FOREIGN_WINDOW_VULKAN) };

pub const SDL_HINT_VIDEO_WIN_D3DCOMPILER: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_WIN_D3DCOMPILER) };

pub const SDL_HINT_VIDEO_X11_FORCE_EGL: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_X11_FORCE_EGL) };

pub const SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR) };

pub const SDL_HINT_VIDEO_X11_NET_WM_PING: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_X11_NET_WM_PING) };

pub const SDL_HINT_VIDEO_X11_WINDOW_VISUALID: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_X11_WINDOW_VISUALID) };

pub const SDL_HINT_VIDEO_X11_XINERAMA: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_X11_XINERAMA) };

pub const SDL_HINT_VIDEO_X11_XRANDR: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_X11_XRANDR) };

pub const SDL_HINT_VIDEO_X11_XVIDMODE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEO_X11_XVIDMODE) };

pub const SDL_HINT_WAVE_FACT_CHUNK: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WAVE_FACT_CHUNK) };

pub const SDL_HINT_WAVE_RIFF_CHUNK_SIZE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WAVE_RIFF_CHUNK_SIZE) };

pub const SDL_HINT_WAVE_TRUNCATION: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WAVE_TRUNCATION) };

pub const SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING) };

pub const SDL_HINT_WINDOWS_ENABLE_MENU_MNEMONICS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOWS_ENABLE_MENU_MNEMONICS) };

pub const SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP) };

pub const SDL_HINT_WINDOWS_FORCE_MUTEX_CRITICAL_SECTIONS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOWS_FORCE_MUTEX_CRITICAL_SECTIONS) };

pub const SDL_HINT_WINDOWS_FORCE_SEMAPHORE_KERNEL: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOWS_FORCE_SEMAPHORE_KERNEL) };

pub const SDL_HINT_WINDOWS_INTRESOURCE_ICON: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOWS_INTRESOURCE_ICON) };
pub const SDL_HINT_WINDOWS_INTRESOURCE_ICON_SMALL: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOWS_INTRESOURCE_ICON_SMALL) };

pub const SDL_HINT_WINDOWS_NO_CLOSE_ON_ALT_F4: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOWS_NO_CLOSE_ON_ALT_F4) };

pub const SDL_HINT_WINDOWS_USE_D3D9EX: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOWS_USE_D3D9EX) };

pub const SDL_HINT_WINDOWS_DPI_AWARENESS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOWS_DPI_AWARENESS) };

pub const SDL_HINT_WINDOWS_DPI_SCALING: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOWS_DPI_SCALING) };

pub const SDL_HINT_WINDOW_FRAME_USABLE_WHILE_CURSOR_HIDDEN: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOW_FRAME_USABLE_WHILE_CURSOR_HIDDEN) };

pub const SDL_HINT_WINDOW_NO_ACTIVATION_WHEN_SHOWN: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINDOW_NO_ACTIVATION_WHEN_SHOWN) };

pub const SDL_HINT_WINRT_HANDLE_BACK_BUTTON: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINRT_HANDLE_BACK_BUTTON) };

pub const SDL_HINT_WINRT_PRIVACY_POLICY_LABEL: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINRT_PRIVACY_POLICY_LABEL) };

pub const SDL_HINT_WINRT_PRIVACY_POLICY_URL: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_WINRT_PRIVACY_POLICY_URL) };

pub const SDL_HINT_X11_FORCE_OVERRIDE_REDIRECT: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_X11_FORCE_OVERRIDE_REDIRECT) };

pub const SDL_HINT_XINPUT_ENABLED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_XINPUT_ENABLED) };

pub const SDL_HINT_DIRECTINPUT_ENABLED: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_DIRECTINPUT_ENABLED) };

pub const SDL_HINT_XINPUT_USE_OLD_JOYSTICK_MAPPING: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_XINPUT_USE_OLD_JOYSTICK_MAPPING) };

pub const SDL_HINT_AUDIO_INCLUDE_MONITORS: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_AUDIO_INCLUDE_MONITORS) };

pub const SDL_HINT_X11_WINDOW_TYPE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_X11_WINDOW_TYPE) };

pub const SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE) };

pub const SDL_HINT_VIDEODRIVER: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_VIDEODRIVER) };

pub const SDL_HINT_AUDIODRIVER: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_AUDIODRIVER) };

pub const SDL_HINT_KMSDRM_DEVICE_INDEX: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_KMSDRM_DEVICE_INDEX) };

pub const SDL_HINT_TRACKPAD_IS_TOUCH_ONLY: &str =
    unsafe { from_c_define(crate::bindings::SDL_HINT_TRACKPAD_IS_TOUCH_ONLY) };

pub fn sdl_set_hint(hint: impl AsRef<str>, value: impl AsRef<str>) {
    let hint_c = CString::new(hint.as_ref()).unwrap();
    let value_c = CString::new(value.as_ref()).unwrap();
    unsafe { SDL_SetHint(hint_c.as_ptr(), value_c.as_ptr()) };
}
