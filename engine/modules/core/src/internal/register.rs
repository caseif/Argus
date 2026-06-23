use crate::{LifecycleStage};
use linkme::distributed_slice;

#[distributed_slice]
pub static REGISTERED_MODULES: [ModuleRegistration];

#[derive(Clone, Debug)]
pub struct ModuleRegistration {
    pub id: &'static str,
    pub depends_on: &'static [&'static str],
    pub entry_point: fn(LifecycleStage),
}

impl PartialEq for ModuleRegistration {
    fn eq(&self, other: &Self) -> bool {
        self.id == other.id
    }
}
