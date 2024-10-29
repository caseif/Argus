/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

pub struct Dirtiable<T> {
    value: T,
    dirty: bool,
}

impl<T> Dirtiable<T> {
    pub fn new(value: T) -> Dirtiable<T> {
        Self {
            value,
            dirty: false
        }
    }

    pub fn read(&mut self) -> ValueAndDirtyFlag<T>
    where T: Clone {
        let res = ValueAndDirtyFlag::<T>::of(self.value.clone(), self.dirty);
        self.dirty = false;
        res
    }

    pub fn read_ref(&mut self) -> ValueAndDirtyFlag<&T> {
        let res = ValueAndDirtyFlag::<&T>::of(&self.value, self.dirty);
        self.dirty = false;
        res
    }

    pub fn peek(&self) -> ValueAndDirtyFlag<T>
    where T: Clone {
        ValueAndDirtyFlag::<T>::of(self.value.clone(), self.dirty)
    }

    pub fn peek_ref(&self) -> ValueAndDirtyFlag<&T> {
        ValueAndDirtyFlag::<&T>::of(&self.value, self.dirty)
    }

    pub fn set(&mut self, value: T) {
        self.value = value;
        self.dirty = true;
    }

    pub fn update<F: Fn(&T) -> T>(&mut self, func: F) {
        self.value = func(&self.value);
        self.dirty = true;
    }
}

/*impl<T> From<T> for Dirtiable<T> {
    fn from(value: T) -> Self {
        Dirtiable::new(value)
    }
}*/

impl<T: Default> Default for Dirtiable<T> {
    fn default() -> Self {
        Self {
            value: T::default(),
            dirty: false,
        }
    }
}

pub struct ValueAndDirtyFlag<T> {
    pub value: T,
    pub dirty: bool,
}

impl<T> ValueAndDirtyFlag<T> {
    pub fn of(value: T, dirty: bool) -> Self {
        ValueAndDirtyFlag { value, dirty }
    }

    pub fn as_ref(&self) -> ValueAndDirtyFlag<&T> {
        ValueAndDirtyFlag { value: &self.value, dirty: self.dirty }
    }
}
