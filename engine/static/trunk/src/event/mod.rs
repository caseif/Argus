use std::{any, sync::{Arc, Mutex}, collections::VecDeque};

use lowlevel::collections::indexed_list::{SyncBufferedIndexedList, Index};

use crate::LOGGER;
use crate::engine::EngineHandle;

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
        listeners: &Arc<SyncBufferedIndexedList<ArgusEventHandler>>) {
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

    while !queue_copy.is_empty() {
        let event = queue_copy.pop_front().unwrap();
        for listener in &*listeners.values() {
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
        return listeners.add(self.get_next_callback_index(), listener);
    }

    pub fn unregister_event_handler(&mut self, id: Index) {
        if !self.event_state.update_event_listeners.try_remove(id) {
            self.event_state.render_event_listeners.remove(id);
        }
    }

    pub(crate) fn deinit_event_handlers(&mut self) {
        self.event_state.update_event_listeners.clear();
        self.event_state.render_event_listeners.clear();
    }
}
