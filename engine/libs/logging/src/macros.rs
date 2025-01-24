#[macro_export]
#[doc(hidden)]
macro_rules! log_internal {
    ($method:ident, $logger:expr, $format:literal, $($arg:tt)*) => {
        $crate::Logger::$method(::std::ops::Deref::deref(&$logger), format!($format, $($arg)*))
    };
    ($method:ident, $logger:expr, $message:expr) => {
        $crate::Logger::$method(::std::ops::Deref::deref(&$logger), $message)
    };
}

#[macro_export]
macro_rules! severe {
    ($logger:expr, $($arg:tt)*) => {
        $crate::log_internal!(severe, $logger, $($arg)*)
    }
}

#[macro_export]
macro_rules! warn {
    ($logger:expr, $($arg:tt)*) => {
        $crate::log_internal!(warn, $logger, $($arg)*)
    };
}

#[macro_export]
macro_rules! info {
    ($logger:expr, $($arg:tt)*) => {
        $crate::log_internal!(info, $logger, $($arg)*)
    }
}

#[macro_export]
macro_rules! debug {
    ($logger:expr, $($arg:tt)*) => {
        $crate::log_internal!(debug, $logger, $($arg)*)
    }
}

#[macro_export]
macro_rules! trace {
    ($logger:expr, $($arg:tt)*) => {
        $crate::log_internal!(trace, $logger, $($arg)*)
    }
}

#[macro_export]
macro_rules! crate_logger {
    ($name:ident, $channel:tt) => {
        pub(crate) static $name: ::std::sync::LazyLock<$crate::Logger> =
            ::std::sync::LazyLock::new(|| {
                $crate::Logger::new($channel).expect("Failed to create crate logger")
            });
    }
}
