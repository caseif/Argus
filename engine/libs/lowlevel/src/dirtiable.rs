pub struct ValueAndDirtyFlag<V: Copy> {
    value: V,
    dirty: bool
}

impl<V: Copy> ValueAndDirtyFlag<V> {
    pub fn new(value: V) -> Self {
        ValueAndDirtyFlag { value, dirty: false }
    }
}

impl<V: Copy + Default> Default for ValueAndDirtyFlag<V> {
   fn default() -> Self {
       ValueAndDirtyFlag { value: Default::default(), dirty: false }
   }
}

struct Dirtiable<V: Copy> {
    value: V,
    dirty: bool
}

impl<V: Copy + PartialEq> Dirtiable<V> {
    pub fn new(value: &V) -> Self {
        Dirtiable { value: *value, dirty: false }
    }

    fn is_same_val(&self, new_val: &V) -> bool {
        *new_val == self.value
    }

    pub fn read(&mut self) -> ValueAndDirtyFlag<V> {
        let old_dirty = self.dirty;
        self.dirty = false;
        return ValueAndDirtyFlag { value: self.value, dirty: old_dirty };
    }

    pub fn peek(&self) -> &V {
        &self.value
    }

    pub fn set_quietly(&mut self, value: V) {
        self.value = value;
    }
}

impl<V: Copy + Default + PartialEq> Default for Dirtiable<V> {
    fn default() -> Self {
        Dirtiable { value: Default::default(), dirty: false }
    }
}

impl<V: Copy + PartialEq> Into<Dirtiable<V>> for (V,) {
    fn into(self) -> Dirtiable<V> {
        Dirtiable::new(&self.0)
    }
}
