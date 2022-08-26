use std::fmt::Display;

const LEVEL_FATAL: &str = "FATAL";
const LEVEL_WARN: &str = "WARN";
const LEVEL_INFO: &str = "INFO";
const LEVEL_DEBUG: &str = "DEBUG";

pub struct Logger {
    realm: String
}

impl Logger {
    pub fn new(realm: &str) -> Self {
        Logger { realm: realm.to_string() }
    }
    
    fn print<S: Display>(&self, level: &str, message: S) {
        println!("[{}][{}] {}", self.realm, level, message);
    }

    pub fn fatal<S: Display>(&self, message: S) {
        self.print(LEVEL_FATAL, message);
    }

    pub fn fatal_if<S: Display>(&self, cond: bool, message: S) {
        if !cond {
            self.print(LEVEL_FATAL, message);
            std::process::exit(1);
        }
    }

    pub fn warn<S: Display>(&self, message: S) {
        self.print(LEVEL_WARN, message);
    }

    pub fn info<S: Display>(&self, message: S) {
        self.print(LEVEL_INFO, message);
    }

    pub fn debug<S: Display>(&self, message: S) {
        #[feature(debug_assertions)]
        self.print(LEVEL_DEBUG, message);
    }
}
