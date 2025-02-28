use crate::{LifecycleStage};
use linkme::distributed_slice;

#[distributed_slice]
pub static REGISTERED_MODULES: [ModuleRegistration];

#[derive(Clone, Debug, PartialEq)]
pub struct ModuleRegistration {
    pub id: &'static str,
    pub depends_on: &'static [&'static str],
    pub entry_point: fn(LifecycleStage),
}
