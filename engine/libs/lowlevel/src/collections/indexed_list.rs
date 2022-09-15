use std::collections::VecDeque;
use std::sync::{Mutex, MutexGuard};

use crate::logging::Logger;

pub type Index = u64;

#[derive(Clone)]
pub struct IndexedValue<T> {
    pub id: Index,
    pub value: T
}

// This struct defines the list alongside two mutation queues and a shared
// mutex. In this way, it facilitates a thread-safe callback list wherein
// the callbacks themselves may modify the list, i.e. while the list is
// being iterated.
pub struct SyncBufferedIndexedList<T> {
    logger: &'static Logger,
    list: Mutex<Vec<IndexedValue<T>>>,
    add_queue: Mutex<VecDeque<IndexedValue<T>>>,
    rm_queue: Mutex<VecDeque<Index>>
}

fn remove_from_indexed_vector<T>(vector: &mut Vec<IndexedValue<T>>, id: Index) -> bool {
    let initial_len = vector.len();
    vector.retain(|el| el.id != id);
    return vector.len() < initial_len;
}

impl<T> SyncBufferedIndexedList<T> {
    pub fn new(logger: &'static Logger) -> Self {
        SyncBufferedIndexedList {
            logger,
            list: Mutex::new(vec![]),
            add_queue: Mutex::new(VecDeque::new()),
            rm_queue: Mutex::new(VecDeque::new())
        }
    }

    pub fn values(&self) -> MutexGuard<Vec<IndexedValue<T>>> {
        return self.list.lock().unwrap();
    }

    pub fn flush(&self) {
        let mut add_queue = self.add_queue.lock().unwrap();
        let mut rm_queue = self.rm_queue.lock().unwrap();

        if add_queue.is_empty() && rm_queue.is_empty() {
            return;
        }

        let mut list = self.list.lock().unwrap();

        while !rm_queue.is_empty() {
            let id = rm_queue.pop_front().unwrap();
            if !remove_from_indexed_vector(&mut list, id) {
                self.logger.warn(format!("Client attempted to unregister unknown callback {}", id).as_str());
            }
        }

        while !add_queue.is_empty() {
            list.push(add_queue.pop_front().unwrap());
        }
    }

    pub fn add(&self, index: Index, el: T) -> Index {
        self.add_queue.lock().unwrap().push_back(IndexedValue { id: index, value: el });

        return index;
    }

    pub fn remove(&self, index: Index) {
        self.rm_queue.lock().unwrap().push_back(index);
    }

    pub fn try_remove(&self, index: Index) -> bool{
        let present = self.list.lock().unwrap().iter().find(|el| el.id == index).is_some();

        if present {
            self.remove(index);
        }

        return present;
    }

    pub fn clear(&self) {
        self.list.lock().unwrap().clear();
    }
}
