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
use std::ops::Deref;

pub type Version = u32;

#[derive(Debug)]
pub struct Versioned<T> {
    version: Version,
    value: T,
}

impl<T> Versioned<T> {
    pub fn new(value: T) -> Self {
        Self {
            version: 0,
            value,
        }
    }

    pub fn version(&self) -> Version {
        self.version
    }

    pub fn is_version(&self, version: Version) -> bool {
        self.version == version
    }

    pub fn as_cloned(&self) -> T where T: Clone {
        self.value.clone()
    }

    pub fn get_if_stale(&self, version: Version) -> Option<&T> {
        if self.version != version {
            Some(self.as_ref())
        } else {
            None
        }
    }

    pub fn copy_if_stale(&mut self, src: &Self) -> bool where T: Clone {
        if self.version != src.version {
            self.value = src.value.clone();
            true
        } else {
            false
        }
    }

    pub fn set(&mut self, value: T) {
        self.value = value;
        self.bump_version();
    }

    pub fn update<F: Fn(&T) -> T>(&mut self, func: F) {
        self.value = func(&self.value);
        self.bump_version();
    }

    pub fn update_in_place<F: Fn(&mut T)>(&mut self, func: F) {
        func(&mut self.value);
        self.bump_version();
    }

    fn bump_version(&mut self) {
        if cfg!(debug_assertions) && self.version == u32::MAX {
            self.version = 0;
        } else {
            self.version += 1;
        }
    }
}

impl<T> AsRef<T> for Versioned<T> {
    fn as_ref(&self) -> &T {
        &self.value
    }
}

impl<T> Deref for Versioned<T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        &self.value
    }
}

impl<T: Default> Default for Versioned<T> {
    fn default() -> Self {
        Self::new(T::default())
    }
}
