use std::error::Error;
use std::fmt::{Display, Formatter};

#[derive(Debug)]
pub struct EngineError {
    description: String,
}

impl EngineError {
    pub fn new(description: impl Into<String>) -> Self {
        Self { description: description.into() }
    }
}

impl Display for EngineError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Engine error: {}", self.description)
    }
}

impl Error for EngineError {
}
