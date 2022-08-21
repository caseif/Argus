struct RefCountable<T> {
    value: T,
    refcount: usize
}

impl<T> RefCountable<T> {
    pub fn new(value: T) -> Self {
        RefCountable { value, refcount: 1 }
    }

    pub fn acquire(&mut self) {
        self.refcount += 1;
    }

    pub fn release(&mut self) -> usize {
        self.refcount -= 1;
        return self.refcount;
    }
}

impl<T: Default> Default for RefCountable<T> {
    fn default() -> Self {
        RefCountable { value: Default::default(), refcount: 1 }
    }
}
