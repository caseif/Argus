use crate::ScriptManager;
use argus_scripting_bind::{FundamentalType, ObjectType, ReflectiveArgumentsError, ScriptCallbackEntryPoint, ScriptCallbackRef, WrappedObject, WrappedScriptCallback};
use std::ptr;
use std::sync::{Arc, Mutex};

pub fn create_struct_object_wrapper(ty: ObjectType, obj: &[u8])
    -> Result<WrappedObject, ReflectiveArgumentsError> {
    assert_eq!(ty.ty, FundamentalType::Object);

    let size = ty.size;
    let mut wrapper = WrappedObject::new(ty, size);
    unsafe { wrapper.copy_from_slice(obj); }
    Ok(wrapper)
}

pub fn create_ref_object_wrapper(ty: ObjectType, reference: &'static (), pointed_to_size: usize)
    -> Result<WrappedObject, ReflectiveArgumentsError> {
    assert_eq!(ty.ty, FundamentalType::Reference);
    let mut ty_with_size = ty;
    ty_with_size.primary_type.as_mut().unwrap().size = pointed_to_size;
    let mut wrapper = WrappedObject::new(ty_with_size, size_of::<&()>());
    unsafe { wrapper.store_raw(reference).unwrap(); }
    Ok(wrapper)
}

pub fn create_int_object_wrapper(ty: ObjectType, val: i64)
    -> Result<WrappedObject, ReflectiveArgumentsError>{
    let is_signed = ty.ty.is_int() || ty.ty == FundamentalType::Enum;
    let size = ty.size;

    if ty.ty == FundamentalType::Enum {
        assert!(ty.base_type_name.is_some());
        let mgr = ScriptManager::instance();
        let bindings = mgr.get_bindings();
        let enum_def = bindings.get_enum_by_type_id(ty.base_type_id.as_ref().unwrap())
            .expect("Tried to create WrappedObject with unbound enum ty");
        if !enum_def.all_ordinals.contains(&val) {
            return Err(ReflectiveArgumentsError::new(
                format!("Unknown ordinal {} for enum ty {}", val, enum_def.name)
            ));
        };
    }

    let mut wrapper = WrappedObject::new(ty.clone(), size);
    match ty.size {
        1 => {
            if is_signed {
                wrapper.store_value(val as i8)
            } else {
                wrapper.store_value(val as u8)
            }
        }
        2 => {
            if is_signed {
                wrapper.store_value(val as i16)
            } else {
                wrapper.store_value(val as u16)
            }
        }
        4 => {
            if is_signed {
                wrapper.store_value(val as i32)
            } else {
                wrapper.store_value(val as u32)
            }
        }
        8 => {
            if is_signed {
                wrapper.store_value(val)
            } else {
                wrapper.store_value(val as u64)
            }
        }
        _ => {
            panic!(); // should have been caught during binding
        }
    }.expect("Failed to store value with integer type");

    Ok(wrapper)
}

pub fn create_float_object_wrapper(ty: ObjectType, val: f64)
    -> Result<WrappedObject, ReflectiveArgumentsError> {
    let size = ty.size;

    let mut wrapper = WrappedObject::new(ty, size);

    match size {
        4 => {
            wrapper.store_value(val as f32).unwrap();
        }
        8 => {
            wrapper.store_value(val).unwrap();
        }
        _ => {
            panic!(); // should have been caught during binding
        }
    }

    Ok(wrapper)
}

pub fn create_bool_object_wrapper(ty: ObjectType, val: bool)
    -> Result<WrappedObject, ReflectiveArgumentsError> {
    let size = ty.size;
    assert!(size >= size_of::<bool>());

    let wrapper = WrappedObject::wrap(val);

    Ok(wrapper)
}

pub fn create_enum_object_wrapper(ty: ObjectType, val: i64)
-> Result<WrappedObject, ReflectiveArgumentsError> {
    create_int_object_wrapper(ty, val)
}

pub fn create_string_object_wrapper(
    ty: ObjectType,
    s: &str
) -> Result<WrappedObject, ReflectiveArgumentsError> {
    assert_eq!(
        ty.ty,
        FundamentalType::String,
        "Cannot create object wrapper (string-specific overload called for non-string-typed value)",
    );

    Ok(WrappedObject::wrap(s))
}

pub fn create_callback_object_wrapper(
    ty: ObjectType,
    entry_point: ScriptCallbackEntryPoint,
    userdata: Arc<Mutex<dyn ScriptCallbackRef>>,
) -> Result<WrappedObject, ReflectiveArgumentsError> {
    assert_eq!(ty.ty,
               FundamentalType::Callback,
               "Cannot create object wrapper (callback-specific overload called for \
         non-callback-typed value)",
    );

    let mut wrapper = WrappedObject::new(ty, size_of::<WrappedScriptCallback>());
    unsafe { wrapper.store_raw(WrappedScriptCallback { entry_point, userdata }).unwrap(); }

    Ok(wrapper)
}

pub fn destruct_wrapped_object(obj_type: &ObjectType, ptr: *mut ()) {
    match obj_type.ty {
        FundamentalType::Empty => {
            // no-op
        }
        FundamentalType::Object => {
            // for complex value types we indirectly use the copy/move
            // constructors

            assert!(obj_type.base_type_id.is_some());

            let mgr = ScriptManager::instance();
            let bindings = mgr.get_bindings();
            let bound_type = bindings.get_type_by_type_id(obj_type.base_type_id.as_ref().unwrap())
                .expect("Tried to destruct wrapped object with unbound struct ty");
            assert!(bound_type.dropper.is_some());
            bound_type.dropper.as_ref().unwrap()(ptr);
        }
        FundamentalType::Callback => {
            unsafe { ptr::drop_in_place(ptr.cast::<WrappedScriptCallback>()) };
        }
        FundamentalType::Vec |
        FundamentalType::VecRef => {
            todo!()
        }
        _ => {
            // no-op
        }
    }
}
