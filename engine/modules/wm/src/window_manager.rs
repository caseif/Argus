use std::cell::RefCell;
use std::ops::{Deref, DerefMut};
use std::{cell, ptr};
use std::sync::{LazyLock, Mutex, OnceLock};
use std::sync::atomic::{AtomicBool, AtomicU32, Ordering};
use std::time::Duration;
use bitflags::bitflags;
use dashmap::DashMap;
use dashmap::mapref::one::{Ref, RefMut};
use fragile::Fragile;
use sdl3::{EventPump, EventSubsystem, Sdl, VideoSubsystem};
use argus_core::dispatch_event;
use argus_scripting_bind::script_bind;
use argus_util::math::{Vector2f, Vector2i, Vector2u};
use crate::{is_wm_module_initialized, Canvas, Window, WindowEvent, WindowEventType, WindowStateFlags, LOGGER};
use crate::gl_manager::GlManager;
use crate::window::dispatch_window_event;

use sdl3::event::Event as SdlEvent;
use sdl3::event::WindowEvent as SdlWindowEvent;
use sdl3::sys::video::SDL_WINDOW_INPUT_FOCUS;
use sdl3::video::{FullscreenType, Window as SdlWindow, WindowBuilder};
use argus_logging::warn;

static INSTANCE: LazyLock<WindowManager> = LazyLock::new(WindowManager::new);

/// @brief A callback which constructs a Canvas associated with a given
///        Window.
pub type CanvasCtor = dyn Fn(&mut Window) -> Box<dyn Canvas> + Send + Sync;

#[script_bind(ref_only)]
pub struct WindowManager {
    sdl: OnceLock<Fragile<Sdl>>,
    sdl_video_ss: OnceLock<Fragile<VideoSubsystem>>,
    sdl_event_ss: OnceLock<Fragile<EventSubsystem>>,
    sdl_event_pump: OnceLock<Fragile<RefCell<EventPump>>>,
    gl_manager: OnceLock<Fragile<GlManager>>,

    canvas_ctor: OnceLock<Box<CanvasCtor>>,
    window_create_flags: AtomicU32,

    windows: DashMap<String, Window>,
    window_handle_map: DashMap<u32, String>,
}

bitflags! {
    #[derive(Clone, Copy, Debug)]
    pub struct WindowCreationFlags: u32 {
        const None = 0x0;
        const OpenGL = 0x1;
        const Vulkan = 0x2;
        const Metal = 0x4;
        const DirectX = 0x8;
        const WebGPU = 0x10;
        const GraphicsApiMask = 0x1f;
        const Transient = 0x80000000;
    }
}

#[script_bind]
impl WindowManager {
    pub(crate) fn new() -> WindowManager {
        Self {
            sdl: OnceLock::new(),
            sdl_video_ss: OnceLock::new(),
            sdl_event_ss: OnceLock::new(),
            sdl_event_pump: OnceLock::new(),
            gl_manager: OnceLock::new(),
            canvas_ctor: OnceLock::new(),
            window_create_flags: AtomicU32::new(WindowCreationFlags::None.bits()),
            windows: DashMap::new(),
            window_handle_map: DashMap::new(),
        }
    }

    #[script_bind]
    pub fn instance() -> &'static WindowManager {
        INSTANCE.deref()
    }

    pub(crate) fn init_sdl(&self) -> Result<&Sdl, String> {
        let sdl = sdl3::init().map_err(|err| format!("Failed to initialize SDL: {:?}", err))?;
        self.sdl.set(Fragile::new(sdl)).map_err(|_| "SDL was already initialized!".to_owned())?;
        self.sdl_video_ss.set(Fragile::new(self.get_sdl()?.video().unwrap()))
            .expect("SDL video subsystem was already initialized");
        self.sdl_event_ss.set(Fragile::new(self.get_sdl()?.event().unwrap()))
            .expect("SDL event subsystem was already initialized");
        self.sdl_event_pump.set(Fragile::new(RefCell::new(self.get_sdl()?.event_pump().unwrap())))
            .unwrap_or_else(|_| panic!("SDL event pump was already initialized"));
        self.gl_manager.set(Fragile::new(
            GlManager::new(self.sdl_video_ss.get().unwrap().get().clone())
        ))
            .unwrap_or_else(|_| panic!("OpenGL manager was already initialized"));
        Ok(self.sdl.get().unwrap().get())
    }

    pub fn get_sdl(&self) -> Result<&Sdl, String> {
        self.sdl.get().ok_or_else(|| "SDL was not initialized!".to_owned()).and_then(|sdl| {
            sdl.try_get().map_err(|_| {
                "SDL may only be accessed by the thread which initialized it".to_owned()
            })
        })
    }

    pub(crate) fn get_sdl_video_ss(&self) -> Result<&VideoSubsystem, String> {
        self.sdl_video_ss.get().ok_or_else(|| "SDL was not initialized!".to_owned()).and_then(|video| {
            video.try_get().map_err(|_| {
                "SDL may only be accessed by the thread which initialized it".to_owned()
            })
        })
    }

    pub fn get_sdl_event_ss(&self) -> Result<&EventSubsystem, String> {
        self.sdl_event_ss.get().ok_or_else(|| "SDL was not initialized!".to_owned()).and_then(|event| {
            event.try_get().map_err(|_| {
                "SDL may only be accessed by the thread which initialized it".to_owned()
            })
        })
    }

    pub fn get_sdl_event_pump(&self) -> Result<cell::Ref<EventPump>, String> {
        self.sdl_event_pump.get().ok_or_else(|| "SDL was not initialized!".to_owned())
            .and_then(|pump| {
                pump.try_get().map_err(|_| {
                    "SDL may only be accessed by the thread which initialized it".to_owned()
                })
            })
            .map(|pump| {
                pump.borrow()
            })
    }

    pub fn get_sdl_event_pump_mut(&self) -> Result<cell::RefMut<EventPump>, String> {
        self.sdl_event_pump.get().ok_or_else(|| "SDL was not initialized!".to_owned())
            .and_then(|pump| {
                pump.try_get().map_err(|_| {
                    "SDL may only be accessed by the thread which initialized it".to_owned()
                })
            })
            .map(|pump| {
                pump.borrow_mut()
            })
    }

    pub fn get_gl_manager(&self) -> Result<&GlManager, String> {
        let Some(mgr) = self.gl_manager.get() else {
            return Err("Cannot get GL interface: SDL is not yet initialized".to_owned());
        };
        Ok(mgr.get())
    }

    /// @brief Sets the callbacks used to construct and destroy a Canvas
    ///        during the lifecycle of a Window.
    ///
    /// The constructor will be invoked when the Window is constructed or
    /// shortly thereafter, and the resulting Canvas will be associated
    /// with the Window for the duration of its lifespan.
    ///
    /// The destructor will be invoked when a Window receives a close
    /// request or shortly thereafter. The destructor should deinitialize
    /// and deallocate the Canvas passed to it.
    ///
    /// @param ctor The constructor used to create Canvases.
    /// @param dtor The destructor used to deinitialize and deallocate
    ///        Canvases.
    pub fn set_canvas_ctor(
        &self,
        ctor: Box<CanvasCtor>,
    ) {
        if self.canvas_ctor.set(ctor).is_err() {
            panic!("Cannot set canvas constructor more than once");
        }
    }

    /// @brief Sets the window creation flags globally.
    ///
    /// @param flags The flags to be used for all created windows.
    pub fn set_window_creation_flags(&self, flags: WindowCreationFlags) {
        self.window_create_flags.store(flags.bits(), Ordering::Relaxed);
    }

    /// @brief Creates a new Window.
    ///
    /// @param id The unique identifier of the Window.
    /// @param parent The Window which is parent to the new one, or
    ///        `nullptr` if the window does not have a parent..
    ///
    /// @warning Not all platforms may support multiple
    ///          \link Window Windows \endlink.
    ///
    /// @remark A Canvas will be implicitly created during construction
    ///         of a Window.
    pub fn create_window(&self, id: impl Into<String>) ->
        Result<RefMut<String, Window>, String> {
        if !is_wm_module_initialized() {
            return Err("Cannot create window before wm module is initialized.".to_owned());
        }

        let _create_flags = WindowCreationFlags::from_bits_truncate(
            self.window_create_flags.load(Ordering::Relaxed)
        );

        //if create_flags.contains(WindowCreationFlags::Transient) &&
        //    get_current_lifecycle_stage() > LifecycleStage::Init {
        //    return Err("Transient window may not be created after Init stage".to_owned());
        //}

        let id_str: String = id.into();

        let state = WindowStateFlags::Undefined;

        let close_callback = None;

        let mut window = Window {
            handle: None,
            id: id_str.clone(),
            canvas: None,
            properties: Default::default(),
            content_scale: Vector2f::new(1.0, 1.0),
            close_callback,
            state: Mutex::new(state),
            is_close_request_pending: AtomicBool::new(false),
            cur_resolution: Default::default(),
            cur_refresh_rate: 0.0,
            refcount: Default::default(),
            create_flags: WindowCreationFlags::from_bits_truncate(
                self.window_create_flags.load(Ordering::Relaxed)
            ),
        };
        let prim_disp = self.get_sdl_video_ss()?.get_primary_display().map_err(|err| err.to_string())?;
        window.set_display_affinity(prim_disp.try_into()?);

        window.canvas = match self.canvas_ctor.get() {
            Some(ctor) => Some(ctor(&mut window)),
            None => {
                warn!(
                    LOGGER,
                    "No canvas callbacks were set - new window will not have associated canvas!",
                );
                None
            }
        };

        self.windows.insert(id_str.clone(), window);

        Ok(self.windows.get_mut(&id_str).unwrap())
    }

    pub(crate) fn create_platform_window(
        &self,
        id: impl AsRef<str>,
        title: impl AsRef<str>,
        width: u32,
        height: u32,
        position: Option<Vector2i>,
        flags: WindowCreationFlags,
    ) -> Result<SdlWindow, String> {
        let mut builder = WindowBuilder::new(
            self.get_sdl_video_ss()?,
            title.as_ref(),
            width,
            height,
        );
        if let Some(pos) = position {
            builder.position(pos.x, pos.y);
        } else {
            builder.position_centered();
        }
        builder
            .set_window_flags(SDL_WINDOW_INPUT_FOCUS as u32)
            .resizable()
            .hidden();

        let gfx_api_bits = flags & WindowCreationFlags::GraphicsApiMask;
        if gfx_api_bits.bits().count_ones() > 1 {
            panic!("Only one graphics API may be set during window creation");
        }

        if flags.contains(WindowCreationFlags::OpenGL) {
            builder.opengl();
        } else if flags.contains(WindowCreationFlags::Vulkan) {
            builder.vulkan();
        } else if flags.contains(WindowCreationFlags::Metal) {
            if cfg!(any(target_os = "macos", target_os = "ios")) {
                builder.metal_view();
            } else {
                panic!("Metal contexts are not supported on non-Apple platforms");
            }
        } else if flags.contains(WindowCreationFlags::DirectX) {
            panic!("DirectX contexts are not supported at this time");
        } else if flags.contains(WindowCreationFlags::WebGPU) {
            panic!("WebGPU contexts are not supported at this time");
        }

        let window = builder.build()
            .map_err(|err| format!("Failed to create platform window: {:?}", err))?;
        self.window_handle_map.insert(window.id(), id.as_ref().to_owned());
        Ok(window)
    }

    pub fn register_handle(&self, window_id: impl Into<String>, handle: &SdlWindow) {
        self.window_handle_map.insert(handle.id(), window_id.into());
    }

    pub fn get_window_count(&self) -> usize {
        self.windows.len()
    }

    /// @brief Returns the Window with the specified ID. This will return nullptr
    ///        if a Window with the specified ID does not exist at the time that
    ///        the method is called.
    ///
    /// @return The Window with the specified ID.
    pub fn get_window(&self, id: impl AsRef<str>) -> Option<Ref<String, Window>> {
        self.windows.get(id.as_ref())
    }

    pub fn get_window_mut(&self, id: impl AsRef<str>) -> Option<RefMut<String, Window>> {
        self.windows.get_mut(id.as_ref())
    }

    #[script_bind(rename = "get_window")]
    pub fn get_window_unsafe<'a>(&self, id: &str) -> &'a Window {
        unsafe { &*ptr::from_ref(self.get_window(id).unwrap().deref()) }
    }

    #[script_bind(rename = "get_window_mut")]
    pub fn get_window_mut_unsafe<'a>(&self, id: &str) -> &'a mut Window {
        unsafe { &mut *ptr::from_mut(self.get_window_mut(id).unwrap().deref_mut()) }
    }

    /// @brief Looks up a Window based on its underlying handle.
    ///
    /// @return The Window with the provided handle, or None if the handle
    ///         pointer is not known to the engine.
    pub fn get_window_from_handle(&self, handle: SdlWindow) -> Option<Ref<String, Window>> {
        let id = self.window_handle_map.get(&handle.id())?;
        self.windows.get(id.value())
    }

    pub fn get_window_from_handle_mut(&self, handle: SdlWindow) -> Option<RefMut<String, Window>> {
        let id = self.window_handle_map.get(&handle.id())?;
        self.windows.get_mut(id.value())
    }

    pub fn get_window_from_handle_id(&self, id: u32) -> Option<Ref<String, Window>> {
        let id = self.window_handle_map.get(&id)?;
        self.windows.get(id.value())
    }

    pub(crate) fn reset_window_displays(&self) {
        for item in &self.window_handle_map {
            let window_id = item.value();
            let window = self.windows.get_mut(window_id).unwrap();
            if window.is_closed() {
                continue;
            }

            let new_disp =
                window.handle.as_ref().unwrap().get().get_display().unwrap();
            let mut props = window.properties.write().unwrap();
            props.display.set_quietly(Some(new_disp.try_into().unwrap()));
        }
    }

    fn handle_sdl_window_event(&self, window_id: u32, event: &SdlWindowEvent) {
        let Some(window_id_ref) = self.window_handle_map.get(&window_id) else {
            warn!(LOGGER, "Saw event for unknown window ID {}", window_id);
            return;
        };

        let window_ref = self.windows.get(window_id_ref.value())
            .expect("Window was missing for handle");
        let window = window_ref.value();

        if window.is_closed() {
            return;
        }

        match event {
            SdlWindowEvent::Moved(x, y) => {
                dispatch_event(WindowEvent {
                    subtype: WindowEventType::Move,
                    window: window.id.clone(),
                    resolution: None,
                    position: Some(Vector2i::new(*x, *y)),
                    delta: Default::default(),
                });
            }
            SdlWindowEvent::Resized(width, height) => {
                dispatch_event(WindowEvent {
                    subtype: WindowEventType::Resize,
                    window: window.id.clone(),
                    resolution: Some(Vector2u::new(*width as u32, *height as u32)),
                    position: None,
                    delta: None,
                });
            }
            SdlWindowEvent::Minimized => {
                dispatch_window_event(&window.id, WindowEventType::Minimize);
            }
            SdlWindowEvent::Restored => {
                dispatch_window_event(&window.id, WindowEventType::Restore);
            }
            SdlWindowEvent::FocusGained => {
                dispatch_window_event(&window.id, WindowEventType::Focus);
            }
            SdlWindowEvent::FocusLost => {
                dispatch_window_event(&window.id, WindowEventType::Unfocus);
            }
            SdlWindowEvent::CloseRequested => {
                dispatch_window_event(&window.id, WindowEventType::RequestClose);
            }
            //TODO: handle display scale changed event
            _ => {}
        }
    }

    pub(crate) fn update_windows(&self, delta: Duration) {
        for mut item in self.windows.iter_mut() {
            item.value_mut().update(delta);
        }

        self.reap_windows();
    }

    pub(crate) fn handle_sdl_window_events(&self) -> Result<(), String> {
        self.get_sdl_event_ss()?.flush_events(0, u32::MAX);
        self.get_sdl_event_pump_mut()?.pump_events();

        let events: Vec<SdlEvent> = self.get_sdl_event_ss()
            .map_err(|err| err.to_string())?
            .peek_events(1024);
        for event in events {
            let SdlEvent::Window { window_id, win_event, .. } = event else { continue };
            self.handle_sdl_window_event(window_id, &win_event);
        }

        Ok(())
    }

    pub(crate) fn handle_engine_window_event(&self, event: &WindowEvent) {
        let Some(mut window) = self.windows.get_mut(&event.window) else { return; };

        let state = window.state.get_mut().unwrap();
        // ignore events for uninitialized windows
        if !state.contains(WindowStateFlags::Created) {
        return;
        }

        match event.subtype {
            WindowEventType::RequestClose => {
                state.set(WindowStateFlags::CloseRequested, true);
                state.set(WindowStateFlags::Ready, false);
            }
            WindowEventType::Resize => {
                let res = event.resolution.expect("Resolution missing from window resize event");
                window.cur_resolution.write().unwrap().set(res);
            }
            WindowEventType::Move => {
                let pos = event.position.expect("Position missing from window move event");
                let Some(handle) = window.handle.as_ref() else { return; };
                if handle.get().fullscreen_state() == FullscreenType::Off {
                    // if not in fullscreen mode
                    window.properties.write().unwrap().position.set_quietly(pos);
                }
            }
            _ => {}
        }
    }

    pub(crate) fn reap_windows(&self) {
        let mut reap_ids = Vec::new();
        for window in &self.windows {
            if window.state.lock().unwrap().contains(WindowStateFlags::CloseRequestAcked) {
                reap_ids.push(window.id.clone());
            }
        }

        self.window_handle_map.retain(|_, id| !reap_ids.contains(id));
        self.windows.retain(|id, _| !reap_ids.contains(id));
    }
}
