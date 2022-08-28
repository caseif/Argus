use std::{any, sync::{Arc, Mutex, RwLock}, collections::VecDeque, rc::Rc};

use crate::{LOGGER, engine::Index};
use crate::engine::EngineHandle;
use crate::engine::callback::CallbackList;

pub enum TargetThread {
    Update,
    Render
}

pub(crate) type ArgusEventCallback = fn(&dyn ArgusEvent);

pub trait ArgusEvent {
    fn type_id(&self) -> any::TypeId;
}

pub(crate) struct ArgusEventHandler {
    event_type: any::TypeId,
    callback: ArgusEventCallback
}

pub(crate) fn process_event_queue(event_queue: &Arc<Mutex<VecDeque<Arc<dyn ArgusEvent + Send + Sync>>>>,
        listeners: &Arc<RwLock<CallbackList<ArgusEventHandler>>>) {
    // We copy the queue so that we're not holding onto the mutex while we
    // execute listener callbacks. Otherwise, dispatching an event from a
    // listener would result in deadlock since it wouldn't be able to
    // re-lock the queue.
    let mut queue_copy = {
        let mut bare_queue = event_queue.lock().unwrap();
        let copy = bare_queue.clone();

        bare_queue.clear();

        copy
    };

    let listener_list = (*listeners).read().unwrap();

    while !queue_copy.is_empty() {
        let event = queue_copy.pop_front().unwrap();
        for listener in &listener_list.list {
            if listener.value.event_type == event.type_id() {
                (listener.value.callback)(event.as_ref());
            }
        }
    }
}

impl EngineHandle {
    pub fn dispatch_event<T: ArgusEvent + Send + Sync + 'static>(&mut self, event: T) {
        self.event_state.update_event_queue.lock().unwrap().push_back(Arc::new(event));
    }

    pub fn register_event_handler_with_type<T: 'static>(&mut self, callback: ArgusEventCallback,
            target_thread: TargetThread) -> Index {
        LOGGER.fatal_if(self.is_preinit_done(), "Cannot register event listener before engine initialization.");

        let listeners = match target_thread {
            TargetThread::Update => &self.event_state.update_event_listeners,
            TargetThread::Render => &self.event_state.render_event_listeners
        };

        let listener = ArgusEventHandler { event_type: any::TypeId::of::<T>(), callback };
        return listeners.write().unwrap().add(listener);
    }

    pub fn unregister_event_handler(&mut self, id: Index) {
        if !self.event_state.update_event_listeners.write().unwrap().try_remove(id) {
            self.event_state.render_event_listeners.write().unwrap().remove(id);
        }
    }

    pub(crate) fn deinit_event_handlers(&mut self) {
        self.event_state.update_event_listeners.write().unwrap().list.clear();
        self.event_state.render_event_listeners.write().unwrap().list.clear();
    }
}
