use std::collections::HashMap;

pub type ScriptBindableHandle = u64;

const NULL_HANDLE: ScriptBindableHandle = 0;
const MAX_HANDLE: ScriptBindableHandle = u64::MAX;

pub struct HandleMap {
    handle_to_ptr_map: HashMap<ScriptBindableHandle, (String, usize)>,
    ptr_to_handle_maps: HashMap<String, HashMap<usize, ScriptBindableHandle>>,
    next_handle: ScriptBindableHandle,
}

impl HandleMap {
    pub fn new() -> HandleMap {
        Self {
            handle_to_ptr_map: HashMap::new(),
            ptr_to_handle_maps: HashMap::new(),
            next_handle: 1,
        }
    }

    pub fn get_or_create_sv_handle(&mut self, type_id: impl AsRef<str>, ptr: *mut ())
        -> ScriptBindableHandle {
        if let Some(handle_map) = self.ptr_to_handle_maps.get(type_id.as_ref()) {
            if let Some(handle) = handle_map.get(&(ptr as usize)) {
                return *handle;
            }
        }

        if self.next_handle == MAX_HANDLE {
            // should virtually never happen, even if we assign a billion
            // handles per second it would take around 600 years to get to
            // this point
            panic!("Exhausted script object handles");
        }

        let handle = self.next_handle;
        self.next_handle += 1;
        self.handle_to_ptr_map.insert(handle, (type_id.as_ref().to_owned(), ptr as usize));
        self.ptr_to_handle_maps.entry(type_id.as_ref().to_owned()).or_default()
            .insert(ptr as usize, handle);

        handle
    }

    pub fn deref_sv_handle(&self, handle: ScriptBindableHandle, expected_type_id: impl AsRef<str>)
        -> Option<*mut ()> {
        let Some((ty, ptr)) = self.handle_to_ptr_map.get(&handle) else { return None; };

        if ty != expected_type_id.as_ref() {
            // either memory corruption or someone is doing something nasty
            return None;
        }

        Some(*ptr as *mut ())
    }

    pub fn invalidate_sv_handle(&mut self, type_id: impl AsRef<str>, ptr: *mut ()) {
        let Some(handle_map) = self.ptr_to_handle_maps.get_mut(type_id.as_ref()) else { return; };
        if let Some(handle) = handle_map.get(&(ptr as usize)).cloned() {
            handle_map.remove_entry(&(ptr as usize));
            self.handle_to_ptr_map.remove(&handle);
        }
    }
}

/*pub fn on_object_destroyed(message: &ObjectDestroyedMessage) {
    if (!is_current_thread_update_thread()) {
        return;
    }

    invalidate_sv_handle(message.m_type_id.name(), message.m_ptr);
}

pub fn register_object_destroyed_performer() {
    register_message_performer<ObjectDestroyedMessage>(on_object_destroyed);
}*/
