use std::any::Any;
use std::sync::{Arc, Mutex, RwLock};
use std::sync::atomic::{AtomicBool, AtomicI32, Ordering};
use std::time::Duration;
use bitflags::bitflags;
use fragile::Fragile;
use argus_core::{dispatch_event, get_client_name};
use argus_logging::{debug, info};
use argus_scripting_bind::script_bind;
use argus_util::dirtiable::{Dirtiable, ValueAndDirtyFlag};
use argus_util::math::{Vector2f, Vector2i, Vector2u};
use sdl2::hints::{sdl_set_hint, SDL_HINT_MOUSE_RELATIVE_MODE_WARP};
use sdl2::mouse::{sdl_set_relative_mouse_mode, sdl_show_cursor};
use sdl2::video::*;
use crate::*;

const DEF_WINDOW_DIM: u32 = 300;

pub trait Canvas: Any + Send + Sync {
    fn as_any(&self) -> &dyn Any;

    fn as_any_mut(&mut self) -> &mut dyn Any;
}

/// @brief A callback which operates on a window-wise basis.
pub type WindowCallback = dyn Fn(&mut Window) + Send + Sync + 'static;

bitflags! {
    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    pub struct WindowStateFlags: u32 {
        const Undefined = 0x00;
        const Created = 0x01;
        const Committed = 0x02;
        const Ready = 0x04;
        const Visible = 0x08;
        const CloseRequested = 0x10;
        const CloseRequestAcked = 0x20;
    }
}

/// Represents an individual window on the screen.
///
/// Not all platforms may support multiple windows.
#[script_bind(ref_only)]
pub struct Window {
    /// A handle to the lower-level window represented by this object.
    pub(crate) handle: Option<Fragile<SdlWindow>>,
    /// The unique identifier of the window.
    pub(crate) id: String,
    /// The Canvas associated with this Window.
    ///
    /// This is only set if the Canvas constructor has been set by the module
    /// responsible for implementing canvases.
    pub(crate) canvas: Option<Box<dyn Canvas>>,
    /// The Window parent to this one, if applicable.
    pub(crate) parent: Option<String>,
    /// This Window's child \link Window Windows \endlink, if any.
    pub(crate) children: Vec<Arc<RwLock<Window>>>,
    pub(crate) properties: RwLock<WindowProperties>,
    pub(crate) content_scale: Vector2f,
    /// The callback to be executed upon the Window being closed.
    pub(crate) close_callback: Option<Box<WindowCallback>>,
    /// The state of this Window as a bitfield.
    pub(crate) state: Mutex<WindowStateFlags>,
    pub(crate) is_close_request_pending: AtomicBool,
    pub(crate) cur_resolution: RwLock<Dirtiable<Vector2u>>,
    pub(crate) cur_refresh_rate: u16,
    pub(crate) refcount: AtomicI32,
    pub(crate) create_flags: WindowCreationFlags,
}

#[derive(Debug, Default)]
pub struct WindowProperties {
    pub(crate) title: Dirtiable<String>,
    pub(crate) fullscreen: Dirtiable<bool>,
    pub(crate) display: Dirtiable<Option<Display>>,
    pub(crate) display_mode: Dirtiable<Option<DisplayMode>>,
    pub(crate) windowed_resolution: Dirtiable<Vector2u>,
    pub(crate) position: Dirtiable<Vector2i>,
    pub(crate) vsync: Dirtiable<bool>,
    pub(crate) mouse_capture: Dirtiable<bool>,
    pub(crate) mouse_visible: Dirtiable<bool>,
    pub(crate) mouse_raw_input: Dirtiable<bool>,
}

#[script_bind]
impl Window {
    pub(crate) fn get_handle(&self) -> Option<&SdlWindow> {
        self.handle.as_ref().map(|handle| handle.get())
    }

    pub(crate) fn get_handle_mut(&mut self) -> Option<&mut SdlWindow> {
        self.handle.as_mut().map(|handle| handle.get_mut())
    }

    /// @brief Gets the unique identifier of the Window.
    ///
    /// @return The unique identifier of the Window.
    #[must_use]
    #[script_bind]
    pub fn get_id(&self) -> &str {
        self.id.as_str()
    }

    /// @brief Gets the Canvas associated with the Window.
    ///
    /// @return The Canvas associated with the Window.
    ///
    /// @note This will trigger a panic if a Canvas has not been associated
    ///       with the Window. This can occur if a render module has not been
    ///       requested or if the renderer module is buggy.
    #[must_use]
    pub fn get_canvas(&self) -> Option<&dyn Canvas> {
        match self.canvas.as_ref() {
            Some(c) => Some(c.as_ref()),
            None => None,
        }
    }

    /// @brief Gets the Canvas associated with the Window.
    ///
    /// @return The Canvas associated with the Window.
    ///
    /// @note This will trigger a panic if a Canvas has not been associated
    ///       with the Window. This can occur if a render module has not been
    ///       requested or if the renderer module is buggy.
    #[must_use]
    pub fn get_canvas_mut(&mut self) -> Option<&mut dyn Canvas> {
        match self.canvas.as_mut() {
            Some(c) => Some(c.as_mut()),
            None => None,
        }
    }

    /// @brief Gets whether the Window has been created by the windowing
    ///        manager.
    ///
    /// @return Whether the Window has been created.
    #[must_use]
    pub fn is_created(&self) -> bool {
        self.state.lock().unwrap().contains(WindowStateFlags::Created)
    }

    /// @brief Gets whether the Window is ready for manipulation or interaction.
    ///
    /// @return Whether the Window is ready.
    #[must_use]
    pub fn is_ready(&self) -> bool {
        let state = *self.state.lock().unwrap();
        state.contains(WindowStateFlags::Ready) && !state.contains(WindowStateFlags::CloseRequested)
    }

    /// @brief Gets whether a close request is currently pending for the
    ///            Window.
    ///
    /// This is subtly different from Window#is_closed() in that it will
    /// return true immediately after Window#request_close() is invoked
    /// whereas is_closed will only return true after the window itself has
    /// observed the close request.
    ///
    /// @return Whether the a close request is pending for the Window.
    #[must_use]
    pub fn is_close_request_pending(&self) -> bool {
        self.is_close_request_pending.load(Ordering::Relaxed)
    }

    /// @brief Gets whether the Window is preparing to close.
    ///
    /// Once this is observed to return `true`, the Window object should
    /// not be used again.
    ///
    /// @return Whether the Window is preparing to close.
    #[must_use]
    pub fn is_closed(&self) -> bool {
        self.state.lock().unwrap().contains(WindowStateFlags::CloseRequested)
    }

    /// @brief Creates a new window as a child of this one.
    ///
    /// @return The new child window.
    ///
    /// @note The child window will not be modal to the parent.
    pub fn create_child_window(&mut self, _child_id: impl Into<String>) -> Arc<RwLock<Window>> {
        todo!()
    }

    /// @brief Removes the given Window from this Window's child list.
    ///
    /// @param child The child Window to remove.
    ///
    /// @attention This method does not alter the state of the child
    ///         Window, which must be dissociated from its parent separately.
    pub fn remove_child(&mut self, _child_id: String) {
        todo!()
    }

    /// @brief The primary update callback for a Window.
    ///
    /// @param delta The time in microseconds since the last frame.
    pub fn update(&mut self, delta: Duration) {
        // The initial part of a Window's lifecycle looks something like this:
        //   - Window gets constructed.
        //   - On next render iteration, Window has initial update and sets its
        //       CREATED flag and dispatches an event.
        //   - Renderer picks up the event and initializes itself within the
        //       same render iteration (after applying any properties which have
        //       been configured).
        //   - On subsequent render iterations, window checks if it has been
        //       committed by the client (via Window::commit) and aborts update
        //       if not.
        //   - If committed, Window sets ready flag and continues as normal.
        //   - If any at any point a close request is dispatched to the window,
        //       it will supercede any other initialization steps.
        //
        // By the time the ready flag is set, the Window is guaranteed to be
        // configured and the renderer is guaranteed to have seen the CREATE
        // event and initialized itself properly.

        let mut state = self.state.lock().unwrap();

        if state.contains(WindowStateFlags::CloseRequested) {
            // don't acknowledge close until all references from events are released
            let rc = self.refcount.load(Ordering::Relaxed);
            if rc <= 0 {
                debug!(LOGGER, "Window '{}' acknowledges close request", self.id);
                state.set(WindowStateFlags::CloseRequestAcked, true);
            }
            return; // we forego doing anything else after a close request has been sent
        }

        if !state.contains(WindowStateFlags::Created) {
            debug!(LOGGER, "Window '{}' still needs to be created", self.id);

            let sdl_flags = engine_create_flags_to_sdl_flags(self.create_flags);

            self.handle = Some(Fragile::new(
                SdlWindow::create(
                    get_client_name(),
                    SDL_WINDOWPOS_CENTERED as i32,
                    SDL_WINDOWPOS_CENTERED as i32,
                    DEF_WINDOW_DIM as i32,
                    DEF_WINDOW_DIM as i32,
                    sdl_flags,
                )
                    .expect("Failed to create SDL window")
            ));

            WindowManager::instance().register_handle(&self.id, self.handle.as_ref().unwrap().get());

            state.set(WindowStateFlags::Created, true);

            //TODO: figure out how to handle content scale
            self.content_scale = Vector2f::new(1.0, 1.0);

            info!(LOGGER, "Window '{}' was created", self.id);

            dispatch_event(WindowEvent {
                subtype: WindowEventType::Create,
                window: self.id.clone(),
                resolution: None,
                position: None,
                delta: None,
            });

            return;
        }

        if !state.contains(WindowStateFlags::Committed) {
            return;
        }

        let mut props = self.properties.write().unwrap();
        let title = props.title.read();
        let fullscreen = props.fullscreen.read();
        let display = props.display.read();
        let display_mode = props.display_mode.read();
        let windowed_res = props.windowed_resolution.read();
        let position = props.position.read();
        let mouse_capture = props.mouse_capture.read();
        let mouse_visible = props.mouse_visible.read();
        let mouse_raw_input = props.mouse_raw_input.read();

        if title.dirty {
            self.handle.as_mut().unwrap().get_mut().set_title(title.value);
        }

        if fullscreen.dirty
            || (fullscreen.value && (display.dirty || display_mode.dirty)) {
            if fullscreen.value {
                // switch to fullscreen mode or display/display mode

                let target_display = display.value
                    .expect("Window does not have preferred display");

                let handle = self.handle.as_mut().unwrap().get_mut();

                let disp_off = target_display.get_position();
                handle.set_position(disp_off.x, disp_off.y);
                handle.set_fullscreen(SdlWindowFlags::Fullscreen);
                let sdl_mode = match display_mode.value {
                    Some(mode) => {
                        let cur_mode: SdlDisplayMode = mode.try_into().unwrap();
                        sdl_get_closest_display_mode(
                            target_display.get_index(),
                            &cur_mode,
                        )
                            .expect("Failed to get closest display mode for display")
                    }
                    None => {
                        sdl_get_desktop_display_mode(target_display.get_index())
                            .expect("Failed to get desktop display mode")
                    }
                };

                assert!(sdl_mode.width > 0);
                assert!(sdl_mode.height > 0);

                handle.set_display_mode(&sdl_mode).expect("Failed to set window display mode");

                assert!(sdl_mode.refresh_rate <= u16::MAX as i32, "Refresh rate is too big");
                self.cur_resolution.write().unwrap().set(
                    Vector2u::new(sdl_mode.width as u32, sdl_mode.height as u32)
                );
                self.cur_refresh_rate = sdl_mode.refresh_rate as u16;
            } else {
                assert!(
                    windowed_res.value.x <= i32::MAX as u32 &&
                        windowed_res.value.y <= i32::MAX as u32,
                   "Current windowed resolution is too large"
                );

                let target_disp = display.value
                    .expect("Window does not have preferred display");
                let pos_x: i32 = target_disp.get_position().x + position.value.x;
                let pos_y: i32 = target_disp.get_position().y + position.value.y;

                let handle = self.handle.as_mut().unwrap().get_mut();
                handle.set_fullscreen(SdlWindowFlags::Empty);
                handle.set_position(pos_x, pos_y);
                handle.set_size(windowed_res.value.x as i32, windowed_res.value.y as i32);

                self.cur_resolution.write().unwrap().set(windowed_res.value);
            }
        } else if !fullscreen.value {
            // update windowed positon and/or resolution

            if windowed_res.dirty {
                assert!(
                    windowed_res.value.x <= i32::MAX as u32 &&
                        windowed_res.value.y <= i32::MAX as u32,
                   "Current windowed resolution is too large"
                );

                let handle = self.handle.as_mut().unwrap().get_mut();

                handle.set_size(windowed_res.value.x as i32, windowed_res.value.y as i32);

                self.cur_resolution.write().unwrap().set(windowed_res.value);
            }

            if position.dirty {
                let target_disp = display.value.expect("Window does not have preferred display");
                let pos_x: i32 = target_disp.get_position().x + position.value.x;
                let pos_y: i32 = target_disp.get_position().y + position.value.y;
                self.handle.as_mut().unwrap().get_mut().set_position(pos_x, pos_y);
            }
        }

        if mouse_capture.dirty || mouse_visible.dirty {
            if mouse_capture.value && !mouse_visible.value {
                if mouse_raw_input.dirty {
                    sdl_set_hint(
                        SDL_HINT_MOUSE_RELATIVE_MODE_WARP,
                        if mouse_raw_input.value { "0" } else { "1" }
                    );
                }
                sdl_set_relative_mouse_mode(true).expect("Failed to set relative mouse mode");
            } else {
                self.handle.as_mut().unwrap().get_mut().set_mouse_grab(mouse_capture.value);
                //TODO: not great, would be better to set it per window somehow
                sdl_show_cursor(mouse_visible.value).expect("Failed to set mouse visibile toggle");
            }
        }

        if !state.contains(WindowStateFlags::Ready) {
            state.set(WindowStateFlags::Ready, true);
            debug!(LOGGER, "Window '{}' was marked as ready", self.id);
        }

        if !state.contains(WindowStateFlags::Visible) {
            debug!(LOGGER, "Window '{}' needs to be made visible", self.id);

            self.handle.as_mut().unwrap().get_mut().show();
            state.set(WindowStateFlags::Visible, true);

            debug!(LOGGER, "Window '{}' was made visible for the first time", self.id);
        }

        dispatch_event(WindowEvent {
            subtype: WindowEventType::Update,
            window: self.id.clone(),
            resolution: None,
            position: None,
            delta: Some(delta),
        });
    }

    /// Sets the window title.
    ///
    /// @param title The new window title.
    pub fn set_title(&mut self, title: impl Into<String>) {
        self.properties.write().unwrap().title.set(title.into());
    }

    /// @brief Gets whether the window is currently in fullscreen mode.
    ///
    /// @return The window's fullscreen state.
    #[must_use]
    pub fn is_fullscreen(&self) -> bool {
        self.properties.read().unwrap().fullscreen.peek().value
    }

    /// @brief Sets the fullscreen state of the window.
    ///
    /// Caution: This may not be supported on all platforms.
    ///
    /// @param fullscreen Whether the window is to be displayed in
    ///        fullscreen.
    #[script_bind]
    pub fn set_fullscreen(&mut self, fullscreen: bool) {
        self.properties.write().unwrap().fullscreen.set(fullscreen);
    }

    /// @brief Gets the window's current resolution and whether it has
    ///         changed since the last invocation of this function
    ///
    /// The internal dirty flag for the value will be copied to the
    /// return object and then cleared in the Window object.
    ///
    /// @return The window's current resolution and dirty flag.
    ///
    /// @sa peek_resolution
    pub fn get_resolution(&mut self) -> ValueAndDirtyFlag<Vector2u> {
        self.cur_resolution.write().unwrap().read()
    }

    /// @brief Gets the window's current resolution without affecting itsz
    ///        internal dirty bit.
    ///
    /// If another function reads the window's resolution after this
    /// function runs and the resolution has changed since the last time
    /// get_resolution was called, it will see the change as not yet
    /// having been acknowledged.
    ///
    /// @return The window's current resolution.
    ///
    /// @sa get_resolution
    #[must_use]
    pub fn peek_resolution(&self) -> Vector2u {
        self.cur_resolution.read().unwrap().peek().value
    }

    /// @brief Gets the window's configured windowed resolution.
    ///
    /// @return The windowed resolution.
    #[must_use]
    pub fn get_windowed_resolution(&self) -> Vector2u {
        self.properties.read().unwrap().windowed_resolution.peek().value
    }

    /// @brief Sets the window's windowed resolution.
    ///
    /// @param width The horizontal resolution.
    /// @param height The vertical resolution.
    pub fn set_windowed_resolution(&mut self, width: u32, height: u32) {
        self.properties.write().unwrap().windowed_resolution.set(Vector2u::new(width, height));
    }

    /// @brief Gets whether the window has vertical synchronization
    ///        (vsync) enabled and the state of its dirty flag.
    ///
    /// If the returned dirty flag is set, the vsync flag has been
    /// changed since the last invocation of this function.
    ///
    /// @return Whether the window's vsync flag is set and its dirty
    ///         flag.
    #[must_use]
    pub fn is_vsync_enabled(&self) -> ValueAndDirtyFlag<bool> {
        self.properties.write().unwrap().vsync.read()
    }

    /// @brief Enabled or disabled vertical synchronization (vsync) for
    ///        the window.
    ///
    /// @param enabled Whether vertical synchronization (vsync) should be
    ///        enabled for the window.
    #[script_bind]
    pub fn set_vsync_enabled(&mut self, enabled: bool) {
        self.properties.write().unwrap().vsync.set(enabled)
    }

    /// @brief Sets the position of the window on the screen when in
    ///        windowed mode.
    ///
    /// @param x The new X-coordinate of the window.
    /// @param y The new Y-coordinate of the window.
    ///
    /// @warning This may not be supported on all platforms.
    #[script_bind]
    pub fn set_windowed_position(&mut self, x: i32, y: i32) {
        self.properties.write().unwrap().position.set(Vector2i::new(x, y));
    }

    /// @brief Gets the Display this Window will attempt to show on in
    ///        fullscreen mode.
    ///
    /// If this parameter has not been configured for the Window, it
    /// will default to the primary display or otherwise the first
    /// display available.
    ///
    /// @return The Display to show on.
    #[must_use]
    pub fn get_display_affinity(&self) -> Option<Display> {
        self.properties.read().unwrap().display.peek().value
    }

    /// @brief Sets the Display this Window will attempt to show on in
    ///        fullscreen mode.
    ///
    /// @param display The Display to show on.
    ///
    /// @sa Display::get_available_displays
    pub fn set_display_affinity(&mut self, display: Display) {
        self.properties.write().unwrap().display.set(Some(display.clone()));
    }

    /// @brief Gets the DisplayMode used by the Window while in
    ///        fullscreen mode.
    ///
    /// If this parameter has not been configured for the Window, it will
    /// default to the "best" available mode for its Display.
    ///
    /// The display mode controls parameters such as resolution, refresh
    /// rate, and color depth.
    ///
    /// @return The display mode of the Window.
    #[must_use]
    pub fn get_display_mode(&self) -> DisplayMode {
        match self.properties.read().unwrap().display_mode.peek().value {
            Some(mode) => mode,
            None => self.get_display_affinity().unwrap().get_desktop_display_mode(),
        }
    }

    /// @brief Sets the DisplayMode of this Window while in fullscreen
    ///        mode.
    ///
    /// The display mode controls parameters such as resolution, refresh
    /// rate, and color depth.
    ///
    /// @param mode The display mode of the window.
    ///
    /// @sa Display::get_display_modes
    pub fn set_display_mode(&mut self, mode: DisplayMode) {
        self.properties.write().unwrap().display_mode.set(Some(mode))
    }

    /// @brief Gets whether the mouse cursor should be captured by the
    ///        window while it is focused.
    ///
    /// @return Whether the mouse cursor should be captured.
    #[must_use]
    pub fn is_mouse_captured(&self) -> bool {
        self.properties.read().unwrap().mouse_capture.peek().value
    }

    /// @brief Sets whether the mouse cursor should be captured by the
    ///        window while it is focused.
    ///
    /// @param captured Whether the mouse cursor should be captured.
    pub fn set_mouse_captured(&mut self, captured: bool) {
        self.properties.write().unwrap().mouse_capture.set(captured)
    }

    /// @brief Gets whether the mouse cursor is visible within the
    ///        window.
    ///
    /// @return Whether the mouse cursor is visible.
    #[must_use]
    pub fn is_mouse_visible(&self) -> bool {
        self.properties.read().unwrap().mouse_visible.peek().value
    }

    /// @brief Sets whether the mouse cursor is visible within the
    ///        window.
    ///
    /// @param visible Whether the mouse cursor is visible.
    pub fn set_mouse_visible(&mut self, visible: bool) {
        self.properties.write().unwrap().mouse_visible.set(visible)
    }

    /// @brief Gets whether the raw input from the mouse should be used
    ///        by the window.
    ///
    /// @return Whether the raw mouse input should be used.
    #[must_use]
    pub fn is_mouse_raw_input(&self) -> bool {
        self.properties.read().unwrap().mouse_raw_input.peek().value
    }

    /// @brief Sets whether the raw input from the mouse should be used
    ///        by the window.
    ///
    /// @param raw_input Whether the raw mouse input should be used.
    pub fn set_mouse_raw_input(&mut self, raw_input: bool) {
        self.properties.write().unwrap().mouse_raw_input.set(raw_input)
    }

    /// @brief Gets the content scale of the Window as reported by the
    ///        window manager.
    /// @return The content scale of the Window.
    #[must_use]
    pub fn get_content_scale(&self) -> Vector2f {
        self.content_scale
    }

    /// @brief Sets the WindowCallback to invoke upon this window being
    ///        closed.
    ///
    /// @param callback The callback to be executed.
    pub fn set_close_callback(&mut self, callback: Box<WindowCallback>) {
        self.close_callback = Some(callback);
    }

    /// @brief Commits the window configuration, prompting the engine to
    ///        create it.
    ///
    /// @note This function should be invoked only once.
    pub fn commit(&mut self) {
        if self.create_flags.contains(WindowCreationFlags::Transient) {
            panic!("Committing transient window is not permitted");
        }
        self.state.lock().unwrap().set(WindowStateFlags::Committed, true);
    }

    /// @brief Sends a close request to the window.
    ///
    /// This will inititate the process of closing the window, although this
    /// process will not occur immediately.
    ///
    /// This function is thread-safe.
    pub fn request_close(&mut self) {
        if self.create_flags.contains(WindowCreationFlags::Transient) {
            // allow immediate reaping
            self.state.lock().unwrap().set(WindowStateFlags::CloseRequestAcked, true);
        } else {
            self.is_close_request_pending.store(true, Ordering::Relaxed);
            dispatch_window_event(&self.id, WindowEventType::RequestClose);
        }
    }
}

pub(crate) fn dispatch_window_event(window_id: impl Into<String>, ty: WindowEventType) {
    dispatch_event(WindowEvent {
        subtype: ty,
        window: window_id.into(),
        resolution: None,
        position: None,
        delta: None,
    });
}

fn engine_create_flags_to_sdl_flags(engine_flags: WindowCreationFlags) -> SdlWindowFlags {
    let mut sdl_flags = SdlWindowFlags::Resizable |
        SdlWindowFlags::InputFocus |
        SdlWindowFlags::Hidden;

    let gfx_api_bits = engine_flags & WindowCreationFlags::GraphicsApiMask;
    if gfx_api_bits.bits().count_ones() > 1 {
        panic!("Only one graphics API may be set during window creation");
    }

    if engine_flags.contains(WindowCreationFlags::OpenGL) {
        sdl_flags.set(SdlWindowFlags::OpenGL, true);
    } else if engine_flags.contains(WindowCreationFlags::Vulkan) {
        sdl_flags.set(SdlWindowFlags::Vulkan, true);
    } else if engine_flags.contains(WindowCreationFlags::Metal) {
        if cfg!(any(target_os = "macos", target_os = "ios")) {
            sdl_flags.set(SdlWindowFlags::Metal, true);
        } else {
            panic!("Metal contexts are not supported on non-Apple platforms");
        }
    } else if engine_flags.contains(WindowCreationFlags::DirectX) {
        panic!("DirectX contexts are not supported at this time");
    } else if engine_flags.contains(WindowCreationFlags::WebGPU) {
        panic!("WebGPU contexts are not supported at this time");
    }

    sdl_flags
}
