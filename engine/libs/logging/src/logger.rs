use crate::{LogLevel, LogManager};
use crate::manager::get_log_manager;

pub struct Logger {
    mgr: &'static LogManager,
    channel: String,
}

impl Logger {
    pub fn new(channel: impl Into<String>) -> Result<Self, ()> {
        let Some(mgr) = get_log_manager() else { return Err(()); };
        let logger = Self { mgr, channel: channel.into() };
        Ok(logger)
    }

    pub fn log(&self, level: LogLevel, message: impl Into<String>) {
        let res = self.mgr.stage_message(self.channel.clone(), level, message.into());
        if let Err(msg_obj) = res {
            eprintln!(
                "Failed to submit log message: LogManager is already deinitialized ({:?})",
                msg_obj,
            );
        }
    }

    pub fn trace(&self, message: impl Into<String>) {
        self.log(LogLevel::Trace, message);
    }

    pub fn debug(&self, message: impl Into<String>) {
        self.log(LogLevel::Debug, message);
    }

    pub fn info(&self, message: impl Into<String>) {
        self.log(LogLevel::Info, message);
    }

    pub fn warn(&self, message: impl Into<String>) {
        self.log(LogLevel::Warning, message);
    }

    pub fn severe(&self, message: impl Into<String>) {
        self.log(LogLevel::Severe, message);
    }
}
