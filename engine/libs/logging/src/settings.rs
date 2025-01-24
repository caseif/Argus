use std::fmt::{Display, Formatter};

const MAX_CLOCK_PRECISION: usize = 9;

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Severe,
}

impl Default for LogLevel {
    fn default() -> LogLevel {
        if cfg!(debug_assertions) {
            LogLevel::Debug
        } else {
            LogLevel::Info
        }
    }
}

impl Display for LogLevel {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        let s = match self {
            LogLevel::Severe => "Severe",
            LogLevel::Warning => "Warning",
            LogLevel::Info => "Info",
            LogLevel::Debug => "Debug",
            LogLevel::Trace => "Trace",
        };
        write!(f, "{}", s)
    }
}

#[derive(Clone, Copy, Debug)]
pub enum PreludeComponent {
    LocalTime,
    UtcTime,
    MonotonicTime,
    Level,
    Channel,
}

#[derive(Clone, Copy, Debug)]
pub enum MonotonicPrecision {
    Seconds,
    Milliseconds,
    Microseconds,
    Nanoseconds,
}

#[derive(Clone, Debug)]
pub struct LogSettings {
    pub min_level: LogLevel,
    pub prelude: Vec<PreludeComponent>,
    pub clock_precision: usize,
}

impl Default for LogSettings {
    fn default() -> Self {
        Self {
            min_level: LogLevel::default(),
            prelude: vec![
                PreludeComponent::MonotonicTime,
                PreludeComponent::Channel,
                PreludeComponent::Level,
            ],
            clock_precision: 6,
        }
    }
}

pub(crate) fn validate_settings(settings: LogSettings) -> LogSettings {
    let mut real_settings = settings;
    if real_settings.clock_precision > MAX_CLOCK_PRECISION {
        real_settings.clock_precision = MAX_CLOCK_PRECISION;
    }
    real_settings
}
