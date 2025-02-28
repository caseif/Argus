use fragile::Fragile;
use std::cell::{Ref, RefCell};
use std::collections::HashMap;
use std::hash::Hash;
use std::sync::{Arc, LazyLock};
use std::time::Duration;
use parking_lot::Mutex;

pub type NullaryCallback = dyn Fn();
pub type DeltaCallback = dyn Fn(Duration);

pub(crate) struct BufferedMap<K, V>
where
    K: 'static + Eq + Hash {
    items: LazyLock<Fragile<RefCell<HashMap<K, V>>>>,
    queues: Arc<Mutex<BufferedMapQueues<K, V>>>,
}

struct BufferedMapQueues<K, V>
where
    K: Eq + Hash {
    add_queue: Vec<(K, V)>,
    remove_queue: Vec<K>,
}

impl<K, V> Default for BufferedMapQueues<K, V>
where
    K: Eq + Hash {
    fn default() -> Self {
        Self {
            add_queue: Default::default(),
            remove_queue: Default::default(),
        }
    }
}

impl<K, V> Default for BufferedMap<K, V>
where
    K: Eq + Hash {
    fn default() -> Self {
        Self {
            items: Default::default(),
            queues: Default::default(),
        }
    }
}

impl<K, V> BufferedMap<K, V>
where
    K: Eq + Hash {
    pub(crate) fn new() -> Self {
        Self {
            items: Default::default(),
            queues: Default::default(),
        }
    }

    pub(crate) fn insert(&self, key: K, value: V) {
        self.queues.lock().add_queue.push((key, value));
    }

    pub(crate) fn remove(&self, key: K) {
        self.queues.lock().remove_queue.push(key);
    }

    pub(crate) fn items(&self) -> Ref<HashMap<K, V>> {
        self.items.get().borrow()
    }

    pub(crate) fn drain(&self) -> Vec<(K, V)> {
        self.items.get().borrow_mut().drain().collect::<Vec<_>>()
    }

    pub(crate) fn flush_queues(&self) {
        let mut queues = self.queues.lock();

        let items_fragile = self.items.get();
        let mut items = items_fragile.borrow_mut();

        while let Some((k, v)) = queues.add_queue.pop() {
            items.insert(k, v);
        }
        while let Some(key) = queues.remove_queue.pop() {
            items.remove(&key);
        }
    }
}
