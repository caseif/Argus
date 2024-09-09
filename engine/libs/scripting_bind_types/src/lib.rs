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

use std::any::TypeId;
use std::collections::HashMap;

pub struct BoundStructInfo {
    pub name: &'static str,
    pub type_id: fn() -> TypeId,
    pub size: usize,
}

impl BoundStructInfo {
    pub const fn new(name: &'static str, type_id: fn() -> TypeId, size: usize) -> Self {
        Self { name, type_id, size }
    }
}

pub struct BoundEnumInfo {
    pub name: &'static str,
    pub type_id: fn() -> TypeId,
    pub width: usize,
    pub values: &'static [(&'static str, fn() -> i64)],
}

impl BoundEnumInfo {
    pub const fn new(
        name: &'static str,
        type_id: fn() -> TypeId,
        width: usize,
        values: &'static [(&'static str, fn() -> i64)],
    )
        -> Self {
        Self { name, type_id, width, values }
    }
}

inventory::collect!(BoundStructInfo);
inventory::collect!(BoundEnumInfo);
