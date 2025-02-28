use std::ptr;
use std::sync::{Arc, LazyLock, Mutex};
use argus_logging::warn;
use argus_util::math::{Vector2i, Vector2u, Vector4u};
use sdl2::events::*;
use sdl2::video::*;
use crate::{WindowManager, LOGGER};

#[allow(non_upper_case_globals)]
static g_displays: LazyLock<Arc<Mutex<Vec<Display>>> >=
    LazyLock::new(|| Arc::new(Mutex::new(Vec::new())));

#[derive(Clone, Copy, Debug)]
pub struct DisplayMode {
    pub format: u32,
    pub resolution: Vector2u,
    pub refresh_rate: u16,
    pub color_depth: Vector4u,
}

#[derive(Clone, Debug)]
pub struct Display {
    index: i32,

    name: String,
    position: Vector2i,

    modes: Vec<DisplayMode>,
}

impl TryFrom<SdlDisplayMode> for DisplayMode {
    type Error = String;

    fn try_from(mode: SdlDisplayMode) -> Result<DisplayMode, String> {
        if mode.width <= 0 || mode.height <= 0 {
            return Err("Display mode dimensions must be greater than 0".to_string());
        }

        let masks = mode.format.get_masks()
            .map_err(|err| {
                format!("Failed to query color channels modes for display mode: {}", err)
            })?;

        Ok(DisplayMode {
            resolution: Vector2u::new(mode.width as u32, mode.height as u32),
            refresh_rate: mode.refresh_rate as u16,
            color_depth: Vector4u::new(
                masks.red.count_ones(),
                masks.green.count_ones(),
                masks.blue.count_ones(),
                masks.alpha.count_ones(),
            ),
            format: 0,
        })
    }
}

impl TryFrom<DisplayMode> for SdlDisplayMode {
    type Error = String;

    fn try_from(mode: DisplayMode) -> Result<SdlDisplayMode, String> {
        Ok(SdlDisplayMode {
            format: SdlPixelFormat::try_from(mode.format).map_err(|err| err.to_string())?,
            width: mode.resolution.x as i32,
            height: mode.resolution.y as i32,
            refresh_rate: mode.refresh_rate as i32,
            driverdata: ptr::null_mut(),
        })
    }
}

impl Display {
    pub(crate) fn new(
        index: i32,
        name: impl Into<String>,
        position: Vector2i,
        modes: impl Into<Vec<DisplayMode>>
    ) -> Self {
        Self {
            index,
            name: name.into(),
            position,
            modes: modes.into(),
        }
    }

    #[must_use]
    pub fn get_available_displays() -> Vec<Display> {
        g_displays.lock().unwrap().clone()
    }

    pub(crate) fn get_from_index(index: i32) -> Option<Display> {
        g_displays.lock().unwrap().get(index as usize).cloned()
    }

    #[must_use]
    pub fn get_index(&self) -> i32 {
        self.index
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

    pub(crate) fn get_desktop_display_mode(&self) -> DisplayMode {
        sdl_get_desktop_display_mode(self.index).unwrap().try_into().unwrap()
    }
}

pub(crate) fn init_display() {
    *g_displays.lock().unwrap() = enumerate_displays();

    sdl_add_event_watch(display_callback);
}

fn add_display(display_index: i32) -> Result<Display, String> {
    let display_name = sdl_get_display_name(display_index)
        .map_err(|err| {
            format!("Failed to query name of display {} ({})", display_index, err.get_message())
        })?;

    let bounds = sdl_get_display_bounds(display_index)
        .map_err(|err| {
            format!("Failed to query bounds of display {} ({})", display_index, err.get_message())
        })?;

    let mode_count = sdl_get_num_display_modes(display_index)
        .map_err(|err| {
            format!(
                "Failed to query display modes for display {} ({})",
                display_index,
                err.get_message()
            )
        })?;

    let mut modes = Vec::<DisplayMode>::new();
    for mode_index in 0..mode_count {
        let mode = match sdl_get_display_mode(display_index, mode_index) {
            Ok(mode) => mode,
            Err(err) => {
                warn!(
                    LOGGER,
                    "Failed to query display mode {} for display {}, skipping ({})",
                    mode_index,
                    display_index,
                    err.get_message(),
                );
                continue;
            }
        };

        modes.push(mode.try_into()?);
    }

    Ok(Display {
        index: display_index,
        name: display_name,
        position: Vector2i::new(bounds.x, bounds.y),
        modes,
    })
}

fn enumerate_displays() -> Vec<Display> {
    let Ok(count) = sdl_get_num_video_displays()
    else { panic!("Failed to enumerate displays"); };

    let mut displays: Vec<Display> = Vec::with_capacity(count as usize);
    for i in 0..count {
        displays.push(add_display(i).unwrap());
    }

    displays
}

fn update_displays() {
    let new_displays: Vec<Display> = enumerate_displays();

    WindowManager::instance().reset_window_displays();

    *g_displays.lock().unwrap() = new_displays;
}

fn display_callback(event: &SdlEvent) -> i32 {
    if event.ty != SdlEventType::DisplayEvent {
        return 0;
    }

    let SdlEventData::Display(disp_event) = &event.data
    else { panic!("Display event data not found") };

    if disp_event.ty == SdlDisplayEventType::Connected ||
        disp_event.ty == SdlDisplayEventType::Disconnected {
        update_displays();
    }

    0
}
