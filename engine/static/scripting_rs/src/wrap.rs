use crate::ScriptManager;
use argus_scripting_bind::{IntegralType, ObjectType, ReflectiveArgumentsError, WrappedObject, WrappedScriptCallback};
use std::ptr::copy_nonoverlapping;
use std::{ptr, slice};

pub fn create_object_wrapper_sized(ty: ObjectType, ptr: *const (),
        size: usize)
-> Result<WrappedObject, ReflectiveArgumentsError> {
    let mut wrapper = WrappedObject::new(ty, size);
    unsafe { wrapper.copy_from_slice(slice::from_raw_parts(ptr.cast(), size)) };

    Ok(wrapper)
}

pub fn create_object_wrapper(ty: ObjectType, ptr: *const ())
    -> Result<WrappedObject, ReflectiveArgumentsError> {
    assert_ne!(
        ty.ty,
        IntegralType::String,
        "Cannot create object wrapper for string-typed value - \
         string-specific overload must be used"
    );

    let size = ty.size;
    create_object_wrapper_sized(ty, ptr, size)
}

pub fn create_int_object_wrapper(ty: ObjectType, val: i64)
    -> Result<WrappedObject, ReflectiveArgumentsError>{
    let is_signed = ty.ty.is_int() || ty.ty == IntegralType::Enum;
    let size = ty.size;

    if ty.ty == IntegralType::Enum {
        assert!(ty.type_name.is_some());
        let mgr = ScriptManager::instance();
        let bindings = mgr.get_bindings();
        let enum_def = bindings.get_enum_by_type_id(ty.type_id.as_ref().unwrap())
            .expect("Tried to create WrappedObject with unbound enum ty");
        if !enum_def.all_ordinals.contains(&val) {
            return Err(ReflectiveArgumentsError::new(
                format!("Unknown ordinal {} for enum ty {}", val, enum_def.name)
            ));
        };
    }

    let mut wrapper = WrappedObject::new(ty.clone(), size);
    unsafe {
        match ty.size {
            1 => {
                if is_signed {
                    wrapper.store_value(val as i8);
                } else {
                    wrapper.store_value(val as u8);
                }
            }
            2 => {
                if is_signed {
                    wrapper.store_value(val as i16);
                } else {
                    wrapper.store_value(val as u16);
                }
            }
            4 => {
                if is_signed {
                    wrapper.store_value(val as i32);
                } else {
                    wrapper.store_value(val as u32);
                }
            }
            8 => {
                if is_signed {
                    wrapper.store_value(val);
                } else {
                    wrapper.store_value(val as u64);
                }
            }
            _ => {
                panic!(); // should have been caught during binding
            }
        }
    }

    Ok(wrapper)
}

pub fn create_float_object_wrapper(ty: ObjectType, val: f64)
    -> Result<WrappedObject, ReflectiveArgumentsError> {
    let size = ty.size;

    let mut wrapper = WrappedObject::new(ty, size);

    unsafe {
        match size {
            4 => {
                wrapper.store_value(val as f32);
            }
            8 => {
                wrapper.store_value(val);
            }
            _ => {
                panic!(); // should have been caught during binding
            }
        }
    }

    Ok(wrapper)
}

pub fn create_bool_object_wrapper(ty: ObjectType, val: bool)
    -> Result<WrappedObject, ReflectiveArgumentsError> {
    let size = ty.size;
    assert!(size >= size_of::<bool>());

    let mut wrapper = WrappedObject::new(ty, size);

    unsafe { wrapper.store_value(val) };

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
        IntegralType::String,
        "Cannot create object wrapper (string-specific overload called for non-string-typed value",
    );

    create_object_wrapper_sized(ty, s.as_bytes().as_ptr().cast(), s.len())
}

pub fn create_callback_object_wrapper(
    ty: ObjectType,
    f: WrappedScriptCallback,
) -> Result<WrappedObject, ReflectiveArgumentsError> {
    assert_eq!(ty.ty,
        IntegralType::Callback,
        "Cannot create object wrapper (callback-specific overload called for \
         non-callback-typed value)",
    );

    let mut wrapper = WrappedObject::new(ty, size_of_val(&f));
    unsafe { wrapper.store_internal(f); }

    Ok(wrapper)
}

unsafe fn copy_or_move_wrapped_object(
    copy: bool,
    obj_type: &ObjectType,
    dst: *mut (),
    src: *mut (),
    size: usize
) {
    if obj_type.ty != IntegralType::String {
        assert!(size >= obj_type.size, "Can't copy wrapped object: dest size is too small");
    }

    match obj_type.ty {
        IntegralType::Empty => {
            // no-op
        }
        IntegralType::Object => {
            // for complex value types we indirectly use the copy/move
            // constructors

            assert!(obj_type.type_id.is_some());

            let mgr = ScriptManager::instance();
            let bindings = mgr.get_bindings();
            let bound_type = bindings.get_type_by_type_id(obj_type.type_id.as_ref().unwrap())
                .expect("Tried to copy/move wrapped object with unbound struct ty");
            if copy {
                assert!(bound_type.cloner.is_some());
                bound_type.cloner.as_ref().unwrap()(
                    dst.cast(),
                    src.cast(),
                );
            } else {
                copy_nonoverlapping(dst, src, size);
            }
        }
        IntegralType::Reference => {
            // copy the pointer itself
            copy_nonoverlapping(dst, src, size_of::<*const ()>());
        }
        IntegralType::Vec |
        IntegralType::VecRef |
        IntegralType::Result => {
            todo!()
        }
        _ => {
            // for everything else we bitwise-copy the value
            // note that std::type_index is trivially copyable
            copy_nonoverlapping(dst, src, size);
        }
    }
}

pub unsafe fn copy_wrapped_object(obj_type: &ObjectType, dst: *mut (), src: *const (), size: usize) {
    copy_or_move_wrapped_object(true, obj_type, dst, src.cast_mut(), size);
}

pub unsafe fn move_wrapped_object(obj_type: &ObjectType, dst: *mut (), src: *mut (), size: usize) {
    copy_or_move_wrapped_object(false, obj_type, dst, src, size);
}

pub fn destruct_wrapped_object(obj_type: &ObjectType, ptr: *mut ()) {
    match obj_type.ty {
        IntegralType::Empty => {
            // no-op
        }
        IntegralType::Object => {
            // for complex value types we indirectly use the copy/move
            // constructors

            assert!(obj_type.type_id.is_some());

            let mgr = ScriptManager::instance();
            let bindings = mgr.get_bindings();
            let bound_type = bindings.get_type_by_type_id(obj_type.type_id.as_ref().unwrap())
                .expect("Tried to destruct wrapped object with unbound struct ty");
            assert!(bound_type.dropper.is_some());
            bound_type.dropper.as_ref().unwrap()(ptr);
        }
        IntegralType::Callback => {
            unsafe { ptr::drop_in_place(ptr.cast::<WrappedScriptCallback>()) };
        }
        IntegralType::Vec |
        IntegralType::VecRef => {
            todo!()
        }
        _ => {
            // no-op
        }
    }
}
