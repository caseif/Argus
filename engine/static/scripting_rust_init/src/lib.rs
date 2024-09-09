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

use argus_scripting_bind::{BoundEnumInfo, BoundStructInfo};
use core_rustabi::core_cabi::{LifecycleStage, LIFECYCLE_STAGE_INIT};
use scripting_rustabi::argus::scripting::{bind_type, bind_enum_type, bind_enum_value};

#[no_mangle]
unsafe extern "C" fn update_lifecycle_scripting_rust_init(stage: LifecycleStage) {
    if stage == LIFECYCLE_STAGE_INIT {
        for struct_info in inventory::iter::<BoundStructInfo> {
            let type_id = format!("{:?}", (struct_info.type_id)());
            bind_type(struct_info.name, struct_info.size, type_id.as_str(),
                false, None, None, None)
                .expect("Failed to bind type");
        }

        for enum_info in inventory::iter::<BoundEnumInfo> {
            let type_id = format!("{:?}", (enum_info.type_id)());
            bind_enum_type(enum_info.name, enum_info.width, type_id.as_str())
                .expect("Failed to bind enum type");

            for (val_name, val_fn) in enum_info.values {
                bind_enum_value(type_id.as_str(), val_name, (val_fn)())
                    .expect("Failed to bind enum value");
            }
        }
    }
}