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
pub use argus_scripting_bind_macros::*;
pub use argus_scripting_types::*;
pub use linkme;
use linkme::distributed_slice;
pub use syn;

#[distributed_slice]
pub static BOUND_STRUCT_DEFS: [BoundStructInfo];
#[distributed_slice]
pub static BOUND_ENUM_DEFS: [BoundEnumInfo];
#[distributed_slice]
pub static BOUND_FIELD_DEFS: [(BoundFieldInfo, &'static [fn () -> TypeId])];
#[distributed_slice]
pub static BOUND_FUNCTION_DEFS: [(BoundFunctionInfo, &'static [&'static [fn () -> TypeId]])];
