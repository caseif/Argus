use std::sync::{Arc, Condvar, Mutex};

#[derive(Clone, Default)]
pub struct Semaphore {
    inner: Arc<SemaphoreInner>,
}

#[derive(Default)]
struct SemaphoreInner {
    mutex: Mutex<bool>,
    cv: Condvar,
}

impl Semaphore {
    pub fn is_signaled(&self) -> bool {
        *self.inner.mutex.lock().unwrap()
    }
    
    pub fn notify(&self) {
        let mut signaled = self.inner.mutex.lock().unwrap();
        *signaled = true;
        self.inner.cv.notify_one();
    }
    
    pub fn wait(&self) {
        let mut signaled = self.inner.mutex.lock().unwrap();
        while !*signaled {
            signaled = self.inner.cv.wait(signaled).unwrap();
        }
        *signaled = false;
    }
}
