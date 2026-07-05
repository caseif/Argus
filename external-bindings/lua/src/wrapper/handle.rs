use std::collections::HashMap;

pub type FfiHandle = u64;

const MAX_HANDLE: FfiHandle = u64::MAX;

pub struct FfiHandleMap {
    handle_to_ptr_map: HashMap<FfiHandle, (String, usize, usize)>,
    ptr_to_handle_maps: HashMap<String, HashMap<usize, FfiHandle>>,
    next_handle: FfiHandle,
}

impl FfiHandleMap {
    pub fn new() -> FfiHandleMap {
        Self {
            handle_to_ptr_map: HashMap::new(),
            ptr_to_handle_maps: HashMap::new(),
            next_handle: 1,
        }
    }

    pub fn get_or_create_sv_handle(&mut self, type_id: impl AsRef<str>, size: usize, ptr: *mut ())
        -> FfiHandle {
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
        self.handle_to_ptr_map.insert(handle, (type_id.as_ref().to_owned(), size, ptr as usize));
        self.ptr_to_handle_maps.entry(type_id.as_ref().to_owned()).or_default()
            .insert(ptr as usize, handle);

        handle
    }

    pub fn deref_sv_handle(&self, handle: FfiHandle, expected_type_id: impl AsRef<str>)
                           -> Option<(usize, *mut ())> {
        let (ty, size, ptr) = self.handle_to_ptr_map.get(&handle)?;

        if ty != expected_type_id.as_ref() {
            // either memory corruption or someone is doing something nasty
            return None;
        }

        Some((*size, *ptr as *mut ()))
    }

    #[allow(dead_code)]
    pub fn invalidate_sv_handle(&mut self, type_id: impl AsRef<str>, ptr: *mut ()) {
        let Some(handle_map) = self.ptr_to_handle_maps.get_mut(type_id.as_ref()) else { return; };
        if let Some(handle) = handle_map.get(&(ptr as usize)).cloned() {
            handle_map.remove_entry(&(ptr as usize));
            self.handle_to_ptr_map.remove(&handle);
        }
    }
}
