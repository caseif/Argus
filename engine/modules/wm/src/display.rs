use std::ptr;
use std::sync::{Arc, LazyLock, Mutex};
use argus_logging::warn;
use argus_util::math::{Vector2i, Vector2u, Vector4u};
use sdl3::event::EventWatchCallback;
use sdl3::{VideoSubsystem};
use crate::{WindowManager, LOGGER};

use sdl3::event::DisplayEvent as SdlDisplayEvent;
use sdl3::event::Event as SdlEvent;
use sdl3::pixels::PixelFormat;
use sdl3::video::Display as SdlDisplay;
use sdl3::video::DisplayMode as SdlDisplayMode;

#[allow(non_upper_case_globals)]
static g_displays: LazyLock<Arc<Mutex<Vec<Display>>> >=
    LazyLock::new(|| Arc::new(Mutex::new(Vec::new())));

#[derive(Clone, Debug)]
pub struct Display {
    name: String,
    position: Vector2i,

    modes: Vec<DisplayMode>,

    handle: SdlDisplay,
}

impl Display {
    pub(crate) fn new(
        name: impl Into<String>,
        position: Vector2i,
        modes: impl Into<Vec<DisplayMode>>,
        handle: SdlDisplay,
    ) -> Self {
        Self {
            name: name.into(),
            position,
            modes: modes.into(),
            handle,
        }
    }

    #[must_use]
    pub fn get_available_displays() -> Vec<Display> {
        g_displays.lock().unwrap().clone()
    }

    #[must_use]
    pub fn get_name(&self) -> &str {
        self.name.as_str()
    }

    #[must_use]
    pub fn get_position(&self) -> &Vector2i {
        &self.position
    }

    #[must_use]
    pub fn get_display_modes(&self) -> &[DisplayMode] {
        self.modes.as_slice()
    }

    pub(crate) fn get_desktop_display_mode(&self) -> SdlDisplayMode {
        self.handle.get_mode().unwrap().try_into().unwrap()
    }

    pub(crate) fn get_closest_display_mode(&self, mode: &DisplayMode)
        -> Result<SdlDisplayMode, String> {
        let sdl_mode = SdlDisplayMode::new(
            self.handle,
            // SAFETY: This parameter is unused by get_closest_display_mode.
            // The function simply initializes an instance of the struct with
            // the value SDL_PixelFormat::UNKNOWN which is not unsafe in and
            // of itself.
            unsafe { PixelFormat::unknown() },
            mode.resolution.x as i32,
            mode.resolution.y as i32,
            0.0, // unused
            mode.refresh_rate,
            0, // unused
            0, // unused
            ptr::null_mut::<>(),
        );
        self.handle.get_closest_display_mode(&sdl_mode, false)
            .map_err(|err| format!("Unable to get closest SDL display mode: {}", err.to_string()))
    }
}

#[derive(Clone, Copy, Debug)]
pub struct DisplayMode {
    pub format: u32,
    pub resolution: Vector2u,
    pub refresh_rate: f32,
    pub color_depth: Vector4u,
}

impl TryFrom<SdlDisplayMode> for DisplayMode {
    type Error = String;

    fn try_from(mode: SdlDisplayMode) -> Result<DisplayMode, String> {
        if mode.w <= 0 || mode.h <= 0 {
            return Err("Display mode dimensions must be greater than 0".to_string());
        }

        let masks = mode.format.into_masks()
            .map_err(|err| {
                format!("Failed to query color channels modes for display mode: {}", err)
            })?;

        Ok(DisplayMode {
            resolution: Vector2u::new(mode.w as u32, mode.h as u32),
            refresh_rate: mode.refresh_rate,
            color_depth: Vector4u::new(
                masks.rmask.count_ones(),
                masks.gmask.count_ones(),
                masks.bmask.count_ones(),
                masks.amask.count_ones(),
            ),
            format: 0,
        })
    }
}

pub(crate) fn init_display() {
    *g_displays.lock().unwrap() =
        enumerate_displays(WindowManager::instance().get_sdl_video_ss().unwrap());
    WindowManager::instance().get_sdl_event_ss().unwrap()
        .add_event_watch(DisplayCallback {});
}

impl TryInto<Display> for SdlDisplay {
    type Error = String;
    fn try_into(self) -> Result<Display, Self::Error> {
        let display_name = self.get_name()
            .map_err(|err| {
                format!("Failed to query name of display: {}", err.to_string())
            })?;

        let bounds = self.get_bounds()
            .map_err(|err| {
                format!("Failed to query bounds of display {}: {}", display_name, err.to_string())
            })?;

        let modes: Vec<DisplayMode> = self.get_fullscreen_modes()
            .map_err(|err| {
                format!(
                    "Failed to query display modes for display {}: {}",
                    display_name,
                    err.to_string(),
                )
            })?
            .into_iter()
            .filter_map(|mode| match mode.try_into() {
                Ok(mode) => Some::<DisplayMode>(mode),
                Err(err) => {
                    warn!(
                        LOGGER,
                        "Failed to process mode {}x{}@{} for display {}, skipping ({})",
                        mode.w,
                        mode.h,
                        mode.refresh_rate,
                        display_name,
                        err,
                    );
                    None
                }
            })
            .collect();

        Ok(Display::new(display_name, Vector2i::new(bounds.x, bounds.y), modes, self))
    }
}

fn enumerate_displays(video: &VideoSubsystem) -> Vec<Display> {
    video.displays().unwrap().into_iter().map(|disp| disp.try_into().unwrap()).collect()
}

fn update_displays() {
    let video = WindowManager::instance().get_sdl_video_ss().unwrap();

    let new_displays: Vec<Display> = enumerate_displays(&video);

    WindowManager::instance().reset_window_displays();

    *g_displays.lock().unwrap() = new_displays;
}

struct DisplayCallback {}
impl EventWatchCallback for DisplayCallback {
    fn callback(&mut self, event: SdlEvent) {
        let SdlEvent::Display { display_event, .. } = event else { return; };
        match display_event {
            SdlDisplayEvent::Added |
            SdlDisplayEvent::Removed => {
                update_displays();
            }
            _ => {}
        }
    }
}

