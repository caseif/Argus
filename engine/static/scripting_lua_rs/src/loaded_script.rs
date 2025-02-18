#[derive(Clone, Debug)]
pub(crate) struct LoadedScript {
    pub source: String,
}

impl LoadedScript {
    pub(crate) fn new(source: impl Into<String>) -> Self {
        Self { source: source.into()}
    }
}
