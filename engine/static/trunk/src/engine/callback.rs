use crate::*;
use crate::engine::Index;

use lazy_static::lazy_static;

use std::collections::VecDeque;
use std::sync::Mutex;

lazy_static! {
    static ref g_next_index: Mutex<Index> = Mutex::new(0);
}

pub(crate) struct IndexedValue<T> {
    pub id: Index,
    pub value: T
}

// This struct defines the list alongside two mutation queues and a shared
// mutex. In this way, it facilitates a thread-safe callback list wherein
// the callbacks themselves may modify the list, i.e. while the list is
// being iterated.
pub(crate) struct CallbackList<T> {
    pub list: Vec<IndexedValue<T>>,
    add_queue: VecDeque<IndexedValue<T>>,
    rm_queue: VecDeque<Index>
}

pub(crate) fn remove_from_indexed_vector<T>(vector: &mut Vec<IndexedValue<T>>, id: Index) -> bool{
    let initial_len = vector.len();
    vector.retain(|callback| callback.id != id);
    return vector.len() < initial_len;
}

impl<T> CallbackList<T> {
    pub(crate) fn new() -> Self {
        CallbackList {
            list: vec![],
            add_queue: VecDeque::new(),
            rm_queue: VecDeque::new()
        }
    }

    pub(crate) fn flush(&mut self) {
        if self.add_queue.is_empty() && self.rm_queue.is_empty() {
            return;
        }
        
        while !self.rm_queue.is_empty() {
            let id = self.rm_queue.pop_front().unwrap();
            if !remove_from_indexed_vector(&mut self.list, id) {
                logger.warn(format!("Client attempted to unregister unknown callback {}", id).as_str());
            }
        }

        while !self.add_queue.is_empty() {
            self.list.push(self.add_queue.pop_front().unwrap());
        }
    }

    pub(crate) fn add(&mut self, callback: T) -> Index {
        let index: Index;
        {
            let mut next_index = g_next_index.lock().unwrap();
            index = *next_index;
            *next_index += 1;
        }

        self.add_queue.push_back(IndexedValue { id: index, value: callback });

        return index;
    }

    pub(crate) fn remove(&mut self, index: Index) {
        self.rm_queue.push_back(index);
    }

    pub(crate) fn try_remove(&mut self, index: Index) -> bool{
        let present = (*self.list).iter().find(|callback| callback.id == index).is_some();

        if present {
            self.remove(index);
        }

        return present;
    }

    pub(crate) fn clear(&mut self) {
        self.list.clear();
    }
}
