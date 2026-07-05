pub struct ScriptLoadError {
    pub path: String,
    pub message: String,
}

impl ScriptLoadError {
    pub fn new(path: impl Into<String>, message: impl Into<String>) -> Self {
        Self { path: path.into(), message: message.into() }
    }
}
