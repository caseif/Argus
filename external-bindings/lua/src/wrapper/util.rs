#[macro_export]
macro_rules! log {
    ($state:expr, $level:expr, $format:literal, $($arg:tt)*) => {
        if let Some(callback) = $state.log_callback {
            callback($level, format!($format, $($arg)*).as_str());
        } else {
            println!($format, $($arg)*);
        }
    };
}
