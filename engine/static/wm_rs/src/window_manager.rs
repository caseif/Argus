use std::ops::{Deref, DerefMut};
use std::ptr;
use std::sync::{LazyLock, Mutex, OnceLock};
use std::sync::atomic::{AtomicBool, AtomicU32, Ordering};
use std::time::Duration;
use bitflags::bitflags;
use core_rustabi::argus::core::{dispatch_event, get_current_lifecycle_stage, LifecycleStage};
use dashmap::DashMap;
use dashmap::mapref::one::{Ref, RefMut};
use argus_scripting_bind::script_bind;
use lowlevel_rustabi::argus::lowlevel::{Vector2f, Vector2i, Vector2u};
use sdl2::events::{sdl_get_events, SdlEventData, SdlEventType, SdlWindowEventData, SdlWindowEventType};
use sdl2::video::{SdlWindow, SdlWindowFlags};
use crate::{is_wm_module_initialized, Canvas, Display, Window, WindowEvent, WindowEventType, WindowStateFlags};
use crate::window::dispatch_window_event;

static INSTANCE: LazyLock<WindowManager> = LazyLock::new(WindowManager::new);

/// @brief A callback which constructs a Canvas associated with a given
///        Window.
pub type CanvasCtor = dyn Fn(&mut Window) -> Box<dyn Canvas> + Send + Sync;

#[script_bind(ref_only)]
pub struct WindowManager {
    canvas_ctor: OnceLock<Box<CanvasCtor>>,
    window_create_flags: AtomicU32,

    windows: DashMap<String, Window>,
    window_handle_map: DashMap<usize, String>,
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
    pub fn create_window(&self, id: impl Into<String>, parent: Option<String>) ->
        RefMut<String, Window> {
        assert!(
            is_wm_module_initialized(),
            "Cannot create window before wm module is initialized.",
        );

        let create_flags = WindowCreationFlags::from_bits_truncate(
            self.window_create_flags.load(Ordering::Relaxed)
        );

        if create_flags.contains(WindowCreationFlags::Transient) &&
            get_current_lifecycle_stage() > LifecycleStage::Init {
            panic!("Transient window may not be created after Init stage");
        }

        let id_str: String = id.into();

        let state = WindowStateFlags::Undefined;

        let close_callback = None;

        let mut window = Window {
            handle: None,
            id: id_str.clone(),
            canvas: None,
            parent,
            children: vec![],
            properties: Default::default(),
            content_scale: Vector2f::new(1.0, 1.0),
            close_callback,
            state: Mutex::new(state),
            is_close_request_pending: AtomicBool::new(false),
            cur_resolution: Default::default(),
            cur_refresh_rate: 0,
            refcount: Default::default(),
            create_flags: WindowCreationFlags::from_bits_truncate(
                self.window_create_flags.load(Ordering::Relaxed)
            ),
        };
        window.set_display_affinity(Display::get_from_index(0).unwrap());

        window.canvas = match self.canvas_ctor.get() {
            Some(ctor) => Some(ctor(&mut window)),
            None => {
                println!(
                    "No canvas callbacks were set - new window will not have associated canvas!"
                );
                None
            }
        };

        self.windows.insert(id_str.clone(), window);

        self.windows.get_mut(&id_str).unwrap()
    }

    pub fn register_handle(&self, window_id: impl Into<String>, handle: &SdlWindow) {
        self.window_handle_map.insert(handle.as_addr(), window_id.into());
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
        let id = self.window_handle_map.get(&handle.as_addr())?;
        self.windows.get(id.value())
    }

    pub fn get_window_from_handle_mut(&self, handle: SdlWindow) -> Option<RefMut<String, Window>> {
        let id = self.window_handle_map.get(&handle.as_addr())?;
        self.windows.get_mut(id.value())
    }

    pub(crate) fn reset_window_displays(&self) {
        for item in &self.window_handle_map {
            let window_id = item.value();
            let window = self.windows.get_mut(window_id).unwrap();
            if window.is_closed() {
                continue;
            }

            let new_disp_index =
                window.handle.as_ref().unwrap().get().get_display_index().unwrap();
            if new_disp_index < 0 ||
                new_disp_index >= Display::get_available_displays().len() as i32 {
                println!(
                    "Failed to query new display of window ID {}, things might not work correctly!",
                    window.get_id(),
                );
                continue;
            }
            let mut props = window.properties.write().unwrap();
            let disp = Display::get_from_index(new_disp_index)
                .expect("Could not get display from index");
            props.display.set_quietly(Some(disp));
        }
    }

    fn handle_sdl_window_event(&self, event: &SdlWindowEventData) {
        let handle = SdlWindow::from_id(event.window_id).expect("Could not get SDL window from ID");
        /*if (SDL_GetWindowID(handle) != event->window.windowID) {
            return 0;
        }*/

        let Some(window_id_ref) = self.window_handle_map.get(&handle.as_addr())
        else { return; };
        let window_id = window_id_ref.value();
        let window_ref = self.windows.get(window_id).expect("Window was missing for handle");
        let window = window_ref.value();

        if window.is_closed() {
            return;
        }

        match event.ty {
            SdlWindowEventType::Moved => {
                dispatch_event(WindowEvent {
                    subtype: WindowEventType::Move,
                    window: window_id.clone(),
                    resolution: None,
                    position: Some(Vector2i::new(event.data_1, event.data_2)),
                    delta: Default::default(),
                });
            }
            SdlWindowEventType::Resized => {
                dispatch_event(WindowEvent {
                    subtype: WindowEventType::Resize,
                    window: window_id.clone(),
                    resolution: Some(Vector2u::new(event.data_1 as u32, event.data_2 as u32)),
                    position: None,
                    delta: None,
                });
            }
            SdlWindowEventType::Minimized => {
                dispatch_window_event(window_id, WindowEventType::Minimize);
            }
            SdlWindowEventType::Restored => {
                dispatch_window_event(window_id, WindowEventType::Restore);
            }
            SdlWindowEventType::FocusGained => {
                dispatch_window_event(window_id, WindowEventType::Focus);
            }
            SdlWindowEventType::FocusLost => {
                dispatch_window_event(window_id, WindowEventType::Unfocus);
            }
            SdlWindowEventType::Close => {
                dispatch_window_event(window_id, WindowEventType::RequestClose);
            }
            //TODO: handle display scale changed event when we move to SDL 3
            _ => {}
        }
    }

    pub(crate) fn update_windows(&self, delta: Duration) {
        for mut item in self.windows.iter_mut() {
            item.value_mut().update(delta);
        }

        self.reap_windows();
    }

    pub(crate) fn handle_sdl_window_events(&self) {
        for event in sdl_get_events(SdlEventType::WindowEvent, SdlEventType::WindowEvent) {
            let SdlEventData::Window(event_data) = event.data
            else { panic!("Event data mismatch for window event") };
            self.handle_sdl_window_event(&event_data);
        }
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
                if !handle.get().get_flags().contains(SdlWindowFlags::Fullscreen) {
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
