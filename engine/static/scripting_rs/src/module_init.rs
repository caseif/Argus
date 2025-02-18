use std::any::TypeId;
use std::collections::HashMap;
use std::mem;
use argus_logging::debug;
use argus_scripting_bind::*;
use core_rustabi::argus::core::{get_scripting_parameters, run_on_game_thread, LifecycleStage};
use crate::*;

const INIT_FN_NAME: &str = "init";

fn apply_type_ids(
    ty: &mut ObjectType,
    type_id_getters: &[fn () -> TypeId],
    next_type_id_index: &mut usize
) {
    let mut next_type_id = || -> Result<TypeId, ()> {
        if *next_type_id_index >= type_id_getters.len() {
            return Err(());
        }

        let type_id = type_id_getters[*next_type_id_index]();
        *next_type_id_index += 1;
        Ok(type_id)
    };

    let has_type_id = ty.ty == IntegralType::Object || ty.ty == IntegralType::Enum;
    let type_id_opt = if has_type_id {
        Some(format!(
            "{:?}",
            next_type_id().expect("Ran out of TypeIds while translating object type")
        ))
    } else {
        None
    };
    ty.type_id = type_id_opt;

    if let Some(prim_type) = ty.primary_type.as_mut() {
        apply_type_ids(prim_type, type_id_getters, next_type_id_index);
    }
    if let Some(sec_type) = ty.secondary_type.as_mut() {
        apply_type_ids(sec_type, type_id_getters, next_type_id_index);
    }
    if let Some(callback) = ty.callback_info.as_mut() {
        for param_type in &mut callback.param_types {
            apply_type_ids(param_type, type_id_getters, next_type_id_index);
        }
        apply_type_ids(&mut callback.return_type, type_id_getters, next_type_id_index);
    }
}

#[no_mangle]
pub extern "C" fn update_lifecycle_scripting_rs(stage: LifecycleStage) {
    match stage {
        LifecycleStage::Init => {
            debug!(LOGGER, "Initializing scripting_rs");
            register_script_bindings();
        }
        LifecycleStage::PostInit => {
            // parameter type resolution is deferred to ensure that all
            // types have been registered first
            ScriptManager::instance().get_bindings().resolve_types()
                .expect("Failed to resolve parameter types");

            ScriptManager::instance().apply_bindings_to_all_contexts()
                .expect("Failed to apply bindings to script contexts");

            let scripting_params = get_scripting_parameters();
            if let Some(init_script_uid) = scripting_params.main {
                // run it during the first iteration of the update loop
                run_on_game_thread(Box::new(move || { run_init_script(init_script_uid.clone()); }));
            }
        }
        LifecycleStage::Deinit => {
            ScriptManager::instance().perform_deinit();
        }
        _ => {}
    }
}

pub fn register_script_bindings() {
    debug!(LOGGER, "Struct def count: {}", BOUND_STRUCT_DEFS.len());

    let mut type_defs = HashMap::new();
    let mut enum_defs = HashMap::new();

    for struct_info in BOUND_STRUCT_DEFS {
        debug!(
            LOGGER,
            "Processing struct {} with type {:?}",
            struct_info.name,
            (struct_info.type_id)(),
        );
        let type_id = format!("{:?}", (struct_info.type_id)());
        let type_def = BoundTypeDef::new(
            struct_info.name,
            struct_info.size,
            type_id.as_str(),
            false,
            unsafe { mem::transmute(struct_info.copy_ctor) },
            unsafe { mem::transmute(struct_info.dtor) },
        );

        type_defs.insert(type_id, type_def);
    }

    for enum_info in BOUND_ENUM_DEFS {
        debug!(
            LOGGER,
            "Processing enum {} with type {:?}",
            enum_info.name,
            (enum_info.type_id)(),
        );
        let type_id = format!("{:?}", (enum_info.type_id)());
        let mut enum_def = BoundEnumDef::new(enum_info.name, enum_info.width, type_id.as_str());

        for (val_name, val_fn) in enum_info.values {
            enum_def.add_value(val_name, val_fn())
                .expect("Failed to bind enum value");
        }

        enum_defs.insert(type_id, enum_def);
    }

    for (field_info, type_getters) in BOUND_FIELD_DEFS {
        debug!(LOGGER, "Processing field {}", field_info.name);
        let mut field_parsed_type = serde_json::from_str::<ObjectType>(field_info.type_serial)
            .expect("Invalid field type serial");

        apply_type_ids(&mut field_parsed_type, type_getters, &mut 0);
        let containing_type_id = format!("{:?}", (field_info.containing_type)());
        let type_def = type_defs.get_mut(&containing_type_id).unwrap();
        let field_def = BoundFieldDef::new(
            field_info.name,
            field_parsed_type,
            field_info.accessor,
            Some(field_info.mutator),
        );
        type_def.add_field(field_def).expect("Failed to bind field");
    }

    for (fn_info, type_getters_arr) in BOUND_FUNCTION_DEFS {
        debug!(LOGGER, "Processing function {}", fn_info.name);
        let mut param_parsed_types = fn_info.param_type_serials.iter()
            .map(|serial| serde_json::from_str::<ObjectType>(serial).unwrap())
            .collect::<Vec<_>>();
        let mut ret_parsed_type =
            serde_json::from_str::<ObjectType>(fn_info.return_type_serial).unwrap();

        // first getter is for return type, param type getters start at index 1
        for (i, param_type) in param_parsed_types.iter_mut().enumerate() {
            apply_type_ids(param_type, type_getters_arr[i + 1], &mut 0)
        }
        apply_type_ids(&mut ret_parsed_type, type_getters_arr[0], &mut 0);

        match fn_info.ty {
            FunctionType::Global => {
                let fn_def = BoundFunctionDef::new(
                    fn_info.name.clone(),
                    FunctionType::Global,
                    false,
                    param_parsed_types,
                    ret_parsed_type,
                    fn_info.proxy.clone(),
                );
                ScriptManager::instance().get_bindings().bind_global_function(fn_def)
                    .expect("Failed to bind global function");

                debug!(LOGGER, "Bound global function {}", fn_info.name);
            }
            FunctionType::MemberStatic => {
                let assoc_type_id = format!(
                    "{:?}",
                    fn_info.assoc_type
                        .expect("Associated type was missing for member static function")()
                );
                let type_def = type_defs.get_mut(&assoc_type_id).unwrap();
                let fn_def = BoundFunctionDef::new(
                    fn_info.name.clone(),
                    FunctionType::MemberStatic,
                    false,
                    param_parsed_types,
                    ret_parsed_type,
                    fn_info.proxy.clone(),
                );
                type_def.add_static_function(fn_def)
                    .expect("Failed to bind static member function");

                debug!(
                    LOGGER,
                    "Bound static function {}",
                    get_qualified_function_name(fn_info.ty, fn_info.name, Some(&type_def.name)),
                );
            }
            FunctionType::MemberInstance => {
                let assoc_type_id = format!(
                    "{:?}",
                    fn_info.assoc_type
                        .expect("Associated type was missing for member instance function")()
                );
                let type_def = type_defs.get_mut(&assoc_type_id).unwrap();
                let fn_def = BoundFunctionDef::new(
                    fn_info.name,
                    FunctionType::MemberInstance,
                    fn_info.is_const,
                    param_parsed_types,
                    ret_parsed_type,
                    fn_info.proxy,
                );
                type_def.add_instance_function(fn_def)
                    .expect("Failed to bind instance member function");

                debug!(
                    LOGGER,
                    "Bound instance function {}",
                    get_qualified_function_name(fn_info.ty, fn_info.name, Some(&type_def.name)),
                );
            }
            FunctionType::Extension => {
                panic!("Binding extension functions is not supported at this time");
            }
        }
    }

    for (_, type_def) in type_defs {
        ScriptManager::instance().get_bindings().bind_type(type_def)
            .expect("Failed to bind type");
    }

    for (_, enum_def) in enum_defs {
        ScriptManager::instance().get_bindings().bind_enum(enum_def)
            .expect("Failed to bind enum");
    }
}

fn run_init_script(uid: impl AsRef<str>) {
    let context_handle = match load_script_with_new_context(uid) {
        Ok(handle) => handle,
        Err(err) => {
            panic!("Failed to run init script: {}", err.msg);
        }
    };

    let mgr = ScriptManager::instance();
    let mut context = mgr.get_context_mut(context_handle);
    let init_res = context.value_mut().get_mut().invoke_script_function(INIT_FN_NAME, Vec::new());
    if let Err(err) = init_res {
        panic!("Failed to run init script: {}", err.message);
    }
}
