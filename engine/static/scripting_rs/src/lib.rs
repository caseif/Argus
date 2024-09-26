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
mod wrap;

use argus_scripting_bind::*;
use core_rustabi::core_cabi::{LifecycleStage, LIFECYCLE_STAGE_INIT};
use scripting_rustabi::argus::scripting::*;
use crate::wrap::wrap_object;

fn map_integral_type(ty: IntegralType) -> FfiIntegralType {
    match ty {
        IntegralType::Empty => FfiIntegralType::Void,
        IntegralType::Int8 |
        IntegralType::Int16 |
        IntegralType::Int32 |
        IntegralType::Int64 |
        IntegralType::Int128 |
        IntegralType::Uint8 |
        IntegralType::Uint16 |
        IntegralType::Uint32 |
        IntegralType::Uint64 |
        IntegralType::Uint128 => FfiIntegralType::Integer,
        IntegralType::Float32 |
        IntegralType::Float64 => FfiIntegralType::Float,
        IntegralType::Boolean => FfiIntegralType::Boolean,
        IntegralType::String => FfiIntegralType::String,
        IntegralType::ConstReference |
        IntegralType::MutReference |
        IntegralType::ConstPointer |
        IntegralType::MutPointer => FfiIntegralType::Pointer,
        IntegralType::Vector => FfiIntegralType::Vector,
        IntegralType::Result => FfiIntegralType::Result,
        IntegralType::Object => FfiIntegralType::Struct,
    }
}

fn map_object_type(ty: &ObjectType) -> FfiObjectType {
    let integral_type = map_integral_type(ty.ty);
    FfiObjectType::new(
        integral_type,
        ty.size,
        ty.is_const,
        ty.is_refable.as_ref().map(|f| (f.fn_ptr)()).unwrap_or(false),
        ty.type_id.as_ref().map(|f| (f.fn_ptr)()),
        "", //TODO
        None,
        ty.primary_type.as_ref().map(|inner| map_object_type(inner)),
        ty.secondary_type.as_ref().map(|inner| map_object_type(inner)),
    )
}

#[no_mangle]
extern "C" fn update_lifecycle_scripting_rs(stage: LifecycleStage) {
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

        for field_info in inventory::iter::<BoundFieldInfo> {
            let ty = serde_json::from_str::<ObjectType>(field_info.type_serial)
                .expect("Invalid field type serial");

            let obj_type = map_object_type(&ty);
            bind_member_field(
                (field_info.containing_type)(),
                field_info.name,
                &obj_type,
                Box::new(|inst, ty_param| {
                    let val = unsafe { (field_info.accessor)(inst.cast_mut().cast()) };
                    wrap_object(&ty_param, val.cast())
                }),
                Box::new(|inst, new_val| {
                    unsafe { (field_info.mutator)(inst.cast(), new_val.get_value().cast()); }
                }),
            ).expect("Failed to bind field");
        }
    }
}
