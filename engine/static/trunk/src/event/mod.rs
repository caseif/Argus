pub enum TargetThread {
    Update,
    Render
}

pub(crate) fn process_event_queue(target_thread: TargetThread) {
    //TODO
}

pub(crate) fn flush_event_listener_queues(target_thread: TargetThread) {
    //TODO
}

// WARNING: This method is not thread-safe and assumes that we have
// exclusive access to the event handler callback lists. If you attempt to
// invoke this while other threads might be reading the lists, you will have
// a bad time. This should only ever be used after the engine has spun down.
pub(crate) fn deinit_event_handlers() {
    //TODO
}