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
use std::mem;
use argus_scripting_bind::*;
use core_rustabi::core_cabi::{LifecycleStage, LIFECYCLE_STAGE_INIT};
use scripting_rustabi::argus::scripting::*;

fn map_integral_type(ty: IntegralType) -> FfiIntegralType {
    match ty {
        IntegralType::Empty => FfiIntegralType::Void,
        IntegralType::Int8 |
        IntegralType::Int16 |
        IntegralType::Int32 |
        IntegralType::Int64 |
        IntegralType::Int128 => FfiIntegralType::Integer,
        IntegralType::Uint8 |
        IntegralType::Uint16 |
        IntegralType::Uint32 |
        IntegralType::Uint64 |
        IntegralType::Uint128 => FfiIntegralType::UInteger,
        IntegralType::Float32 |
        IntegralType::Float64 => FfiIntegralType::Float,
        IntegralType::Boolean => FfiIntegralType::Boolean,
        IntegralType::String => FfiIntegralType::String,
        IntegralType::Reference |
        IntegralType::MutReference => FfiIntegralType::Pointer,
        IntegralType::Vec => FfiIntegralType::Vector,
        IntegralType::Result => FfiIntegralType::Result,
        IntegralType::Object => FfiIntegralType::Struct,
        IntegralType::Enum => FfiIntegralType::Enum,
    }
}

fn map_object_type<'a>(
    ty: &'a ObjectType,
    type_id_getters: &[fn () -> TypeId],
    next_type_id_index: &mut usize
)
    -> FfiObjectType<'a> {
    let mut next_type_id = || -> Result<TypeId, ()> {
        if *next_type_id_index >= type_id_getters.len() {
            return Err(());
        }

        let type_id = type_id_getters[*next_type_id_index]();
        *next_type_id_index += 1;
        Ok(type_id)
    };

    let integral_type = map_integral_type(ty.ty);
    let has_type_id = ty.ty == IntegralType::Object || ty.ty == IntegralType::Enum;
    let type_id_opt = if has_type_id {
        Some(format!(
            "{:?}",
            next_type_id().expect("Ran out of TypeIds while translating object type")
        ))
    } else {
        None
    };

    let prim_type_mapped_opt = ty.primary_type.as_ref()
        .map(|inner| map_object_type(inner, type_id_getters, next_type_id_index));
    let sec_type_mapped_opt = ty.secondary_type.as_ref()
        .map(|inner| map_object_type(inner, type_id_getters, next_type_id_index));

    let mut type_id_opt = type_id_opt;
    if ty.ty == IntegralType::Reference ||
        ty.ty == IntegralType::MutReference {
        assert!(prim_type_mapped_opt.is_some());
        type_id_opt = Some(prim_type_mapped_opt.as_ref().unwrap().get_type_id());
    }
    let type_id_opt = type_id_opt;

    FfiObjectType::new(
        integral_type,
        ty.size,
        ty.is_const,
        ty.get_is_refable().unwrap_or(false),
        type_id_opt,
        None,
        prim_type_mapped_opt,
        sec_type_mapped_opt,
        ty.copy_ctor.map(|getter| getter.fn_ptr),
        ty.move_ctor.map(|getter| getter.fn_ptr),
    )
}

#[no_mangle]
pub extern "C" fn update_lifecycle_scripting_rs(stage: LifecycleStage) {
    if stage == LIFECYCLE_STAGE_INIT {
        println!("Running init");
        register_script_bindings();
    }
}

pub fn register_script_bindings() {
    println!("Struct def count: {}", BOUND_STRUCT_DEFS.len());
    for struct_info in BOUND_STRUCT_DEFS {
        println!("Processing struct {}", struct_info.name);
        let type_id = format!("{:?}", (struct_info.type_id)());
        bind_type(
            struct_info.name,
            struct_info.size,
            type_id.as_str(),
            false,
            unsafe { mem::transmute(struct_info.copy_ctor) },
            unsafe { mem::transmute(struct_info.move_ctor) },
            unsafe { mem::transmute(struct_info.dtor) },
        )  
            .expect("Failed to bind type");
    }

    for enum_info in BOUND_ENUM_DEFS {
        let type_id = format!("{:?}", (enum_info.type_id)());
        bind_enum_type(enum_info.name, enum_info.width, type_id.as_str())
            .expect("Failed to bind enum type");

        for (val_name, val_fn) in enum_info.values {
            bind_enum_value(type_id.as_str(), val_name, (val_fn)())
                .expect("Failed to bind enum value");
        }
    }

    for (field_info, type_getters) in BOUND_FIELD_DEFS {
        println!("Processing field {}", field_info.name);
        let field_parsed_type = serde_json::from_str::<ObjectType>(field_info.type_serial)
            .expect("Invalid field type serial");

        let field_ffi_type = map_object_type(&field_parsed_type, type_getters, &mut 0);
        bind_member_field(
            (field_info.containing_type)(),
            field_info.name,
            &field_ffi_type,
            Box::new(|inst, _ty_param| {
                let wrapper = unsafe { (field_info.accessor)(inst.cast_mut().cast()) };

                to_ffi_obj_wrapper(wrapper).unwrap()
            }),
            Box::new(|inst, new_val| {
                unsafe { (field_info.mutator)(inst.cast(), new_val.get_value()); }
            }),
        ).expect("Failed to bind field");
    }

    for (fn_info, type_getters_arr) in BOUND_FUNCTION_DEFS {
        println!("Processing function {}", fn_info.name);
        let param_parsed_types = fn_info.param_type_serials.iter()
            .map(|serial| serde_json::from_str::<ObjectType>(serial).unwrap())
            .collect::<Vec<_>>();
        let ret_parsed_type =
            serde_json::from_str::<ObjectType>(fn_info.return_type_serial).unwrap();

        // first getter is for return type, param type getters start at index 1
        let param_ffi_types: Vec<_> = param_parsed_types.iter()
            .enumerate()
            .map(|(i, p)| map_object_type(p, type_getters_arr[i], &mut 0))
            .collect();
        let ret_ffi_type = map_object_type(&ret_parsed_type, type_getters_arr[0], &mut 0);

        bind_global_function(
            fn_info.name,
            param_ffi_types,
            ret_ffi_type,
            fn_info.proxy,
        ).expect("Failed to bind global function");
    }
}
