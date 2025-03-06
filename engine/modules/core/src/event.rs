use std::any::Any;
use crate::manager::EngineManager;
use crate::Ordering;

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub enum TargetThread {
    Update,
    Render,
}

pub trait ArgusEvent: 'static + Send + Sync {
    fn as_any_ref(&self) -> &dyn Any;
}

pub fn register_event_handler<T: ArgusEvent>(
    handler: impl 'static + Send + Fn(&T),
    target_thread: TargetThread,
    ordering: Ordering,
) -> u64 {
    EngineManager::instance().add_event_handler(handler, target_thread, ordering)
}

pub fn register_boxed_event_handler<T: ArgusEvent>(
    handler: Box<dyn 'static + Send + Fn(&T)>,
    target_thread: TargetThread,
    ordering: Ordering,
) -> u64 {
    EngineManager::instance().add_boxed_event_handler(handler, target_thread, ordering)
}

pub fn unregister_event_handler(index: u64, target_thread: TargetThread) {
    EngineManager::instance().remove_event_handler(index, target_thread);
}

pub fn dispatch_event<T: ArgusEvent>(event: T) {
    EngineManager::instance().push_event::<T>(event);
}