use std::any::Any;
use std::time::Duration;
use lowlevel_rustabi::argus::lowlevel::{Vector2i, Vector2u};
use core_rs::ArgusEvent;

/// @brief A type of WindowEvent.
///
/// @sa WindowEvent
#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub enum WindowEventType {
    Create,
    Update,
    RequestClose,
    Minimize,
    Restore,
    Focus,
    Unfocus,
    Resize,
    Move
}

/// @brief An ArgusEvent pertaining to a Window.
///
/// @sa ArgusEvent
/// @sa Window
pub struct WindowEvent {
    /// @brief The specific \link WindowEventType type \endlink of
    ///        WindowEvent.
    pub subtype: WindowEventType,
    /// @brief The name of the Window associated with the event.
    pub window: String,

    /// @brief The new resolution of the Window.
    ///
    /// @note This is populated only for resize events.
    pub resolution: Option<Vector2u>,

    /// @brief The new position of the Window.
    /// @note This is populated only for move events.
    pub position: Option<Vector2i>,

     /// @brief The \link TimeDelta delta \endlink of the current render
     ///        frame.
     /// @note This is populated only for update events.
    pub delta: Option<Duration>,
}

impl ArgusEvent for WindowEvent {
    fn as_any_ref(&self) -> &dyn Any {
        self
    }
}

impl WindowEvent {
    /// @brief Constructs a new WindowEvent with the given data.
    ///
    /// @param subtype The specific \link WindowEventType type \endlink of
    ///        WindowEvent.
    /// @param window The Window associated with the event.
    /// @param data The new position of resolution of the window following
    ///        the event.
    pub fn new(
        subtype: WindowEventType,
        window: String,
        resolution: Option<Vector2u>,
        position: Option<Vector2i>,
        delta: Option<Duration>
    ) -> Self {
        Self {
            subtype,
            window,
            resolution,
            position,
            delta,
        }
    }
}
