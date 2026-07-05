use std::any::Any;
use std::rc::Rc;
use std::sync::{Arc, Mutex};
use argus_logging::{debug, info, severe, trace, warn};
use fragile::Fragile;
use argus_scripting_bind::*;
use num_enum::TryFromPrimitive;
use parking_lot::ReentrantMutex;
use argus_resman::Resource;
use argus_scripting::*;
use lua::upvalue;
use lua::wrapper::context::{LuaContext, LuaRef, LuaRetval, UserDataContents};
use crate::constants::*;
use crate::loaded_script::LoadedScript;
use crate::LOGGER;

const LUA_BUILTIN_INDEX: &str = "__index";
const LUA_BUILTIN_NEWINDEX: &str = "__newindex";

const LUA_CUSTOM_CONST_PREFIX: &str = "const ";
const LUA_CUSTOM_CLONE_FN: &str = "clone";
const LUA_CUSTOM_EMPTY_REPL: &str = "(empty)";

#[derive(Clone)]
pub struct LuaCallback {
    context: Fragile<Rc<ReentrantMutex<LuaContext>>>,
    callback_ref: Fragile<Rc<LuaRef>>,
}

impl LuaCallback {
    pub fn new(context_rc: Rc<ReentrantMutex<LuaContext>>, index: i32) -> Self {
        let lua_ref = LuaContext::create_ref(&context_rc, index);
        LuaCallback {
            context: Fragile::new(context_rc),
            callback_ref: Fragile::new(Rc::new(lua_ref)),
        }
    }
}

impl ScriptCallbackRef for LuaCallback {
    fn call(&self, params: Vec<WrappedObject>)
        -> Result<WrappedObject, ScriptInvocationError> {
        let context = self.context.get().lock();
        let _guard = context.get_stack_guard();

        self.callback_ref.get().push_to_stack();

        invoke_lua_function_from_stack(self.context.get().clone(), params, None)
    }
}

pub struct LuaLanguagePlugin {
    //TODO
}

impl LuaLanguagePlugin {
    pub(crate) fn new() -> Self {
        Self {}
    }
}

impl ScriptLanguagePlugin for LuaLanguagePlugin {
    fn get_language_name() -> &'static str
    where
        Self: Sized,
    {
        LUA_LANG_NAME
    }

    fn get_media_types() -> &'static [&'static str]
    where
        Self: Sized,
    {
        LUA_MEDIA_TYPES
    }

    fn create_context_data(&mut self) -> Rc<ReentrantMutex<dyn Any>> {
        LuaContext::create(ENGINE_LUA_NAMESPACE, Some(require_callback), Some(log_callback))
    }

    fn load_script(&mut self, script_context: &mut ScriptContext, resource: Resource)
        -> Result<(), ScriptLoadError> {
        let context_rc = script_context.get_plugin_data();
        let context = context_rc.lock();
        let loaded_script = resource.get::<LoadedScript>().unwrap();
        context.downcast_ref::<LuaContext>().unwrap()
            .load_script(resource.get_prototype().uid.to_string(), &loaded_script.source)
            .map(|_| ())
            .map_err(|err| ScriptLoadError {
                resource_uid: err.path,
                message: err.message,
            })
    }

    fn bind_type(&mut self, script_context: &mut ScriptContext, type_def: &BoundTypeDef) {
        let context_rc = script_context.get_plugin_data();
        let context_any = context_rc.lock();
        let context = context_any.downcast_ref::<LuaContext>().unwrap();

        create_type_metatable(context, type_def, false);
        create_type_metatable(context, type_def, true);

        for field_def in type_def.fields.values() {
            bind_type_field(context, &type_def.name, field_def);
        }

        for fn_def in type_def.static_functions.values() {
            bind_type_function(context, &type_def.name, fn_def);
        }

        for fn_def in type_def.instance_functions.values() {
            bind_type_function(context, &type_def.name, fn_def);
        }

        for fn_def in type_def.extension_functions.values() {
            bind_type_function(context, &type_def.name, fn_def);
        }
    }

    fn bind_global_function(
        &mut self,
        script_context: &mut ScriptContext,
        fn_def: &BoundFunctionDef
    ) {
        let context_rc = script_context.get_plugin_data();
        let context_any = context_rc.lock();
        let context = context_any.downcast_ref::<LuaContext>().unwrap();
        bind_global_fn(context, fn_def);
    }

    fn bind_enum(&mut self, script_context: &mut ScriptContext, enum_def: &BoundEnumDef) {
        let context_rc = script_context.get_plugin_data();
        let context = context_rc.lock();
        bind_enum(context.downcast_ref::<LuaContext>().unwrap(), enum_def);
    }

    fn commit_bindings(&mut self, _context: &mut ScriptContext) {
        // no-op
    }

    fn invoke_script_function(
        &mut self,
        script_context: &mut ScriptContext,
        name: &str,
        params: Vec<WrappedObject>,
    ) -> Result<WrappedObject, ScriptInvocationError> {
        let context_rc = script_context.get_plugin_data();
        let context_any = context_rc.lock();
        let context = context_any.downcast_ref::<LuaContext>().unwrap();

        if params.len() > i32::MAX as usize {
            panic!("Too many params to Lua function");
        }

        let _guard = context.get_stack_guard();

        // place Lua function on stack
        context.get_global(name);

        invoke_lua_function_from_stack(context_rc.clone(), params, Some(name))
    }
}

fn wrap_instance_ref(
    context: &LuaContext,
    qual_fn_name: &str,
    param_index: i32,
    type_def: &BoundTypeDef,
    require_mut: bool
) -> Result<WrappedObject, i32> {
    if !context.is_userdata(param_index) {
        return Err(context.push_error(
            format!(
                "Incorrect type provided for parameter {} of function {} (expected {}, actual {})",
                param_index,
                qual_fn_name,
                type_def.name,
                context.get_typename(param_index),
            ),
        ));
    }

    let cur_mt_name = context.get_metatable_name(param_index);

    let expected_mt_name = &type_def.name;
    let expected_mt_name_const = format!(
        "{}{}",
        LUA_CUSTOM_CONST_PREFIX,
        type_def.name,
    );
    let is_correct_mt = cur_mt_name.as_ref().map(|mt| {
        mt == expected_mt_name ||
            (!require_mut && mt == &expected_mt_name_const)
    })
        .unwrap_or(false);

    if !is_correct_mt {
        return Err(context.push_error(format!(
            "Incorrect userdata provided for parameter {} of function {} (expected {}, actual {})",
            param_index,
            qual_fn_name,
            type_def.name,
            cur_mt_name.as_deref().unwrap_or(LUA_CUSTOM_EMPTY_REPL),
        )));
    }

    let udata = context.get_userdata(param_index).unwrap();
    let instance_ptr = match udata {
        UserDataContents::Handle(handle) => {
            match context.open_handle(handle, &type_def.type_id) {
                Some(s) => s,
                None => {
                    return Err(context.push_error(format!(
                        "Invalid handle passed as parameter {} of function {}",
                        param_index,
                        qual_fn_name,
                    )));
                }
            }
        }
        UserDataContents::Blob(blob) => blob,
    };

    let is_const = cur_mt_name.unwrap().starts_with(LUA_CUSTOM_CONST_PREFIX);

    //TODO
    let obj_type = ObjectType {
        ty: FundamentalType::Reference,
        size: size_of::<*const ()>(),
        is_const,
        is_refable: None,
        is_refable_getter: None,
        base_type_id: Some(type_def.type_id.clone()),
        base_type_name: Some(type_def.name.clone()),
        primary_type: Some(Box::new(ObjectType {
            ty: FundamentalType::Object,
            size: type_def.size,
            is_const,
            is_refable: None,
            is_refable_getter: None,
            base_type_id: Some(type_def.type_id.clone()),
            base_type_name: Some(type_def.name.clone()),
            primary_type: None,
            secondary_type: None,
            synthetic_type: None,
            callback_info: None,
            copy_ctor: None,
            dtor: None,
        })),
        secondary_type: None,
        synthetic_type: None,
        callback_info: None,
        copy_ctor: None,
        dtor: None,
    };
    let wrapper_res =
        create_ref_object_wrapper(
            obj_type,
            unsafe { instance_ptr.as_ptr().cast::<()>().as_ref_unchecked() },
            type_def.size
        );
    wrapper_res.map_err(|err|
        context.push_error(format!(
            "Invalid arguments provided for function {}: {}",
            qual_fn_name,
            err.reason,
        ))
    )
}

fn wrap_param(
    context_rc: Rc<ReentrantMutex<LuaContext>>,
    qual_fn_name: &str,
    param_index: i32,
    param_def: ObjectType
) -> Result<WrappedObject, String> {
    let context = context_rc.lock();

    let wrapper_res = if param_def.ty.is_integral() {
        if !context.is_integer(param_index) {
            return Err(format!(
                "Incorrect type provided for parameter {} of function {} \
                     (expected integer{}, actual {})",
                param_index,
                qual_fn_name,
                if param_def.ty == FundamentalType::Enum { "(enum) " } else { "" },
                context.get_typename(param_index),
            ));
        }

        create_int_object_wrapper(param_def, context.get_integer(param_index))
    } else if param_def.ty.is_float() {
        if !context.is_number(param_index) {
            return Err(format!(
                "Incorrect type provided for parameter {} of function {} \
                     (expected number, actual {})",
                param_index,
                qual_fn_name,
                context.get_typename(param_index),
            ));
        }

        create_float_object_wrapper(param_def, context.get_number(param_index))
    } else {
        match param_def.ty {
            FundamentalType::Boolean => {
                if !context.is_boolean(param_index) {
                    return Err(format!(
                        "Incorrect type provided for parameter {} of function {} \
                         (expected boolean, actual {})",
                        param_index,
                        qual_fn_name,
                        context.get_typename(param_index),
                    ));
                }

                create_bool_object_wrapper(
                    param_def,
                    context.is_boolean(param_index),
                )
            }
            FundamentalType::String => {
                if !context.is_string(param_index) {
                    return Err(format!(
                        "Incorrect type provided for parameter {} of function {} \
                     (expected string, actual {})",
                        param_index,
                        qual_fn_name,
                        context.get_typename(param_index),
                    ));
                }

                let s = context.get_string(param_index).unwrap();
                create_string_object_wrapper(
                    param_def,
                    s.as_str()
                )
            }
            FundamentalType::Object |
            FundamentalType::Reference => {
                let underlying_type = if param_def.ty == FundamentalType::Reference {
                    param_def.primary_type.as_ref().unwrap()
                } else {
                    &param_def
                };

                assert!(underlying_type.base_type_name.is_some());
                assert!(underlying_type.base_type_id.is_some());

                if !context.is_userdata(param_index) {
                    return Err(format!(
                        "Incorrect type provided for parameter {} of function {} \
                         (expected userdata, actual {})",
                        param_index,
                        qual_fn_name,
                        context.get_typename(param_index),
                    ));
                }

                let cur_mt_name = context.get_metatable_name(param_index);

                let expected_mt_name = param_def.base_type_name.as_ref().unwrap();
                let expected_mt_name_const = format!(
                    "{}{}",
                    LUA_CUSTOM_CONST_PREFIX,
                    param_def.base_type_name.as_ref().unwrap(),
                );
                let is_correct_mt = cur_mt_name.as_ref().map(|mt| {
                    mt == expected_mt_name ||
                        (param_def.is_const && mt == &expected_mt_name_const)
                })
                    .unwrap_or(false);

                if !is_correct_mt {
                    return Err(format!(
                        "Incorrect userdata provided for parameter {} of function {}{} \
                     (expected {}, actual {})",
                        param_index,
                        qual_fn_name,
                        if param_def.is_const { LUA_CUSTOM_CONST_PREFIX } else { "" },
                        underlying_type.base_type_name.as_ref().unwrap(),
                        cur_mt_name.as_deref().unwrap_or(LUA_CUSTOM_EMPTY_REPL),
                    ));
                }

                let udata = context.get_userdata(param_index).unwrap();
                let data = match udata {
                    UserDataContents::Handle(handle) => {
                        // userdata is storing handle of pointer to struct data
                        let ptr = context.open_handle(
                            handle,
                            underlying_type.base_type_id.as_ref().unwrap()
                        );

                        if ptr.is_none() {
                            return Err(format!(
                                "Invalid handle passed as parameter {} of function {}",
                                param_index,
                                qual_fn_name,
                            ));
                        }

                        ptr.unwrap()
                    }
                    UserDataContents::Blob(blob) => {
                        blob.as_ref()
                    }
                };

                if param_def.ty == FundamentalType::Object {
                    // pass direct pointer so that the struct data is copied
                    // into the WrappedObject
                    create_struct_object_wrapper(param_def, data)
                } else {
                    // copy the pointer itself into the WrappedObject
                    create_ref_object_wrapper(
                        param_def,
                        unsafe { &*data.as_ptr().cast() },
                        data.len()
                    )
                }
            }
            FundamentalType::Callback => {
                /*if (!lua_isfunction(state, param_index)) {
                    return _set_lua_error(state, "Incorrect type provided for parameter "
                                                 + std::to_string(param_index) + " of function " + qual_fn_name
                                                 + " (expected function, actual "
                                                 + luaL_typename(state, param_index) + ")");
                }*/

                let handle = Arc::new(Mutex::new(
                    LuaCallback::new(context_rc.clone(), param_index)
                ));

                let f = |
                    params: Vec<WrappedObject>,
                    data: Arc<Mutex<dyn ScriptCallbackRef>>,
                | {
                    data.lock().unwrap().call(params)
                };

                create_callback_object_wrapper(param_def, f, handle)
            }
            FundamentalType::Vec |
            FundamentalType::VecRef => {
                todo!();
            }
            _ => panic!()
        }
    };

    wrapper_res.map_err(|err| format!(
        "Invalid value passed to for parameter {} of function {} ({})",
        param_index,
        qual_fn_name,
        err.reason,
    ))
}

fn unwrap_int_wrapper(wrapper: WrappedObject) -> i64 {
    assert!(wrapper.get_type().ty.is_integral());

    if wrapper.get_type().ty == FundamentalType::Enum {
        return match wrapper.get_type().size {
            1 => *wrapper.get::<i8>().unwrap() as i64,
            2 => *wrapper.get::<i16>().unwrap() as i64,
            4 => *wrapper.get::<i32>().unwrap() as i64,
            8 => *wrapper.get::<i64>().unwrap(),
            _ => panic!("Bad enum width {} (must be 1, 2, 4, or 8)", wrapper.get_type().size),
        };
    }

    match wrapper.get_type().ty {
        FundamentalType::Int8 => {
            assert_eq!(wrapper.get_type().size, size_of::<i8>());
            *wrapper.get::<i8>().unwrap() as i64
        }
        FundamentalType::Int16 => {
            assert_eq!(wrapper.get_type().size, size_of::<i16>());
            *wrapper.get::<i16>().unwrap() as i64
        }
        FundamentalType::Int32 => {
            assert_eq!(wrapper.get_type().size, size_of::<i32>());
            *wrapper.get::<i32>().unwrap() as i64
        }
        FundamentalType::Int64 => {
            assert_eq!(wrapper.get_type().size, size_of::<i64>());
            *wrapper.get::<i64>().unwrap()
        }
        FundamentalType::Uint8 => {
            assert_eq!(wrapper.get_type().size, size_of::<u8>());
            *wrapper.get::<u8>().unwrap() as i64
        }
        FundamentalType::Uint16 => {
            assert_eq!(wrapper.get_type().size, size_of::<u16>());
            *wrapper.get::<u16>().unwrap() as i64
        }
        FundamentalType::Uint32 => {
            assert_eq!(wrapper.get_type().size, size_of::<u32>());
            *wrapper.get::<u32>().unwrap() as i64
        }
        FundamentalType::Uint64 => {
            assert_eq!(wrapper.get_type().size, size_of::<u64>());
            *wrapper.get::<u64>().unwrap() as i64
        }
        _ => {
            panic!("Bad integer width {} (must be 1, 2, 4, or 8)", wrapper.get_type().size);
        }
    }
}

fn unwrap_float_wrapper(wrapper: WrappedObject) -> f64 {
    assert!(wrapper.get_type().ty.is_float());

    match wrapper.get_type().size {
        4 => {
            *wrapper.get::<f32>().unwrap() as f64
        }
        8 => {
            *wrapper.get::<f64>().unwrap()
        }
        _ => {
            panic!("Bad floating-point width {} (must be 4 or 8)", wrapper.get_type().size);
        }
    }
}

fn unwrap_boolean_wrapper(wrapper: WrappedObject) -> bool {
    assert_eq!(wrapper.get_type().ty, FundamentalType::Boolean);

    *wrapper.get::<bool>().unwrap()
}

fn unwrap_string_wrapper(wrapper: WrappedObject) -> String {
    assert_eq!(wrapper.get_type().ty, FundamentalType::String);

    <String as Wrappable>::unwrap_as_value(&wrapper).unwrap()
}

fn set_metatable(context: &LuaContext, ty: &ObjectType) {
    let mt_name = format!(
        "{}{}",
        if ty.is_const { LUA_CUSTOM_CONST_PREFIX } else { "" },
        ty.base_type_name.as_ref().unwrap(),
    );
    let mt = context.get_metatable_by_name(mt_name);
    assert_ne!(mt, 0); // binding should have failed if type wasn't bound

    context.set_metatable(-2);
}

fn push_value(context: &LuaContext, mut wrapper: WrappedObject) {
    assert_ne!(wrapper.get_type().ty, FundamentalType::Empty);

    if wrapper.get_type().ty.is_integral() {
        context.push_integer(unwrap_int_wrapper(wrapper));
    } else if wrapper.get_type().ty.is_float() {
        context.push_number(unwrap_float_wrapper(wrapper));
    } else {
        match wrapper.get_type().ty {
            FundamentalType::Boolean => {
                context.push_boolean(unwrap_boolean_wrapper(wrapper).into());
            }
            FundamentalType::String => {
                context.push_string(unwrap_string_wrapper(wrapper));
            }
            FundamentalType::Object => {
                assert!(wrapper.get_type().base_type_name.is_some());

                let udata = context.new_userdata(wrapper.get_type().size);
                if let Some(cloner) = wrapper.get_type().copy_ctor {
                    unsafe {
                        (cloner.fn_ptr)(
                            udata.as_mut_slice().as_mut_ptr().cast(),
                            wrapper.get_raw_ptr().cast(),
                        );
                    }
                } else {
                    wrapper.copy_to_slice(udata.as_mut_slice());
                }
                set_metatable(&context, &wrapper.get_type());
            }
            FundamentalType::Reference => {
                assert!(wrapper.get_type().base_type_id.is_some());
                assert!(wrapper.get_type().base_type_name.is_some());

                let obj_ref_ptr: *mut () = unsafe { *wrapper.get_raw_mut_ptr().cast() };

                if !obj_ref_ptr.is_null() {
                    context.new_handle(
                        wrapper.get_type().base_type_id.as_ref().unwrap(),
                        wrapper.get_type().primary_type.as_ref().unwrap().size,
                        obj_ref_ptr,
                    );

                    set_metatable(&context, &wrapper.get_type());
                } else {
                    context.push_nil();
                }
            }
            FundamentalType::Vec |
            FundamentalType::VecRef |
            FundamentalType::Result => {
                todo!();
            }
            _ => {
                panic!("Unsupported type for push");
            }
        }
    }
}

fn invoke_lua_function_from_stack(
    context_rc: Rc<ReentrantMutex<dyn Any>>,
    params: Vec<WrappedObject>,
    fn_name: Option<&str>
) -> Result<WrappedObject, ScriptInvocationError> {
    let params_count = params.len() as i32;

    let context_any = context_rc.lock();
    let context = context_any.downcast_ref::<LuaContext>().unwrap();

    for param in params {
        push_value(&context, param);
    }

    if context.pcall(params_count, 0, 0) != LuaRetval::Ok {
        let err_msg = context.get_string(-1).unwrap();
        context.pop(1); // pop error message
        return Err(ScriptInvocationError {
            fn_name: fn_name.unwrap_or("callback").to_owned(),
            message: err_msg,
        });
    }

    let ty = ObjectType {
        ty: FundamentalType::Empty,
        size: 0,
        is_const: false,
        is_refable: None,
        is_refable_getter: None,
        base_type_id: None,
        base_type_name: None,
        primary_type: None,
        secondary_type: None,
        synthetic_type: None,
        callback_info: None,
        copy_ctor: None,
        dtor: None,
    };
    Ok(WrappedObject::new(ty, 0))
}

fn lua_trampoline(context_rc: Rc<ReentrantMutex<LuaContext>>) -> i32 {
    let context = context_rc.lock();

    let fn_type = FunctionType::try_from_primitive(context.get_integer(upvalue!(1)))
        .expect("Popped unknown function type value from Lua stack");

    let mut type_name: Option<String> = None;
    let mut fn_name_index = 2;
    if fn_type != FunctionType::Global {
        type_name = Some(context.get_string(upvalue!(2)).unwrap());
        fn_name_index = 3;
    }

    let fn_name = context.get_string(upvalue!(fn_name_index)).unwrap();

    let qual_fn_name = get_qualified_function_name(
        fn_type,
        fn_name.clone(),
        type_name.as_ref(),
    );

    let fn_def = {
        let mgr = ScriptManager::instance();
        let bindings = mgr.get_bindings();

        let fn_res = match &fn_type {
            FunctionType::Global =>
                bindings.get_global_function(fn_name),
            FunctionType::MemberInstance =>
                bindings.get_member_instance_function(type_name.as_ref().unwrap(), fn_name),
            FunctionType::Extension =>
                bindings.get_extension_function(type_name.as_ref().unwrap(), fn_name),
            FunctionType::MemberStatic =>
                bindings.get_member_static_function(type_name.as_ref().unwrap(), fn_name),
        };

        if let Err(err) = fn_res {
            let symbol_type_disp = match err.symbol_type {
                SymbolType::Type => "Type",
                SymbolType::Field => "Field",
                SymbolType::Function => "Function",
                _ => "Symbol",
            };

            return context.push_error(format!(
                "{} with name {} is not bound",
                symbol_type_disp,
                err.symbol_name,
            ));
        }

        fn_res.unwrap().clone()
    };

    // parameter count not including instance
    let arg_count = context.get_top();
    let expected_arg_count = fn_def.param_types.len();
    if arg_count as usize != expected_arg_count {
        let mut err_msg = format!(
            "Wrong parameter count provided for function {} (expected {}, actual {})",
            qual_fn_name,
            expected_arg_count,
            arg_count,
        );

        if (fn_type == FunctionType::MemberInstance || fn_type == FunctionType::Extension)
            && expected_arg_count == arg_count as usize + 1 {
            err_msg += " (did you forget to use the colon operator?)";
        }

        return context.push_error(err_msg);
    }

    let mut args = Vec::<WrappedObject>::new();

    /*if fn_type == FunctionType::MemberInstance {
        let mgr = ScriptManager::instance();
        let bindings = mgr.get_bindings();

        // type should definitely be bound since the trampoline function
        // is accessed via the bound type's metatable
        let type_def = bindings.get_type_by_name(type_name.as_ref().unwrap())
            .expect("Failed to find bound type while handling bound instance function");

        //TODO: add safeguard to prevent invocation of functions on non-references

        // 5th param is whether the instance must be mutable, which is
        // the case iff the function is non-const
        let wrapper = match wrap_instance_ref(
            state.deref(),
            &qual_fn_name,
            1,
            type_def,
            !fn_def.is_const,
        ) {
            Ok(w) => w,
            Err(e) => { return e; }
        };
        // if an error occurred, _wrap_instance_ref already sent it to Lua state
        // so we can just bubble the Err value up

        args.push(wrapper);
    }*/

    for i in 0..arg_count {
        // Lua is 1-indexed, also add offset to skip instance parameter if present
        let param_index = i + 1;
        let param_def = fn_def.param_types.get(i as usize).unwrap();

        let wrapper_res = wrap_param(
            context_rc.clone(),
            &qual_fn_name,
            param_index,
            param_def.clone(),
        );
        if let Err(err) = wrapper_res {
            return context.push_error(err);
        }

        args.push(wrapper_res.unwrap());
    }

    let retval_res = (fn_def.proxy)(args);

    if let Err(err) = retval_res {
        return context.push_error(format!(
            "Bad arguments provided to function {} ({})",
            qual_fn_name,
            err.reason,
        ));
    }

    let retval = retval_res.unwrap();

    if retval.get_type().ty != FundamentalType::Empty {
        push_value(&context, retval);
        1
    } else {
        0
    }
}

fn lookup_fn_in_dispatch_table(context: &LuaContext, mt_index: i32, key_index: i32) -> i32 {
    // get value from type's dispatch table instead
    // get type's metatable
    context.get_metatable(mt_index);
    // get dispatch table
    context.get_metatable(-1);
    context.remove(-2);
    // push key onto stack
    context.push(key_index);
    // get value of key from metatable
    context.raw_get(-2);
    context.remove(-2);

    1
}

fn get_native_field_val(
    context_rc: &Rc<ReentrantMutex<LuaContext>>,
    type_name: &str,
    field_name: &str
) -> bool {
    let context = context_rc.lock();

    context.get_stack_guard();

    let real_type_name = type_name.strip_prefix(LUA_CUSTOM_CONST_PREFIX).unwrap_or(type_name);

    let mgr = ScriptManager::instance();
    let bindings = mgr.get_bindings();

    let field_res = bindings.get_field(real_type_name, field_name);
    if field_res.is_err() {
        return false;
    }

    let field = field_res.unwrap();

    let qual_field_name = get_qualified_field_name(real_type_name, field_name);

    // type should definitely be bound since the field is accessed through
    // its associated metatable
    let type_def = bindings.get_type_by_name(real_type_name)
        .expect("Failed to find type while accessing field");

    let Ok(mut inst_wrapper) = wrap_instance_ref(
        &context,
        &qual_field_name,
        1,
        type_def,
        false,
    ) else {
        // some error occurred
        // _wrap_instance_ref already sent error to lua state
        return false;
    };

    let val = field.get_value(&mut inst_wrapper);

    push_value(&context, val);

    true
}

fn lua_type_index_handler(context_rc: Rc<ReentrantMutex<LuaContext>>) -> i32 {
    let context = context_rc.lock();

    let type_name = context.get_metatable_name(1);
    let key = context.get_string(-1);

    if get_native_field_val(
        &context_rc,
        &type_name.unwrap_or("".to_string()),
        &key.unwrap_or("".to_string()),
    ) {
        1
    } else {
        let retval = lookup_fn_in_dispatch_table(&context, 1, 2);
        retval
    }
}

// assumes the value is at the top of the stack
fn set_native_field(
    context_rc: &Rc<ReentrantMutex<LuaContext>>,
    type_name: &str,
    field_name: &str
) -> i32 {
    let context = context_rc.lock();

    let _guard = context.get_stack_guard();

    // only necessary for the error message when the object is const since
    // that's the only time it has the prefix
    let real_type_name = type_name.strip_prefix(LUA_CUSTOM_CONST_PREFIX).unwrap_or(type_name);

    let qual_field_name = get_qualified_field_name(real_type_name, field_name);

    // can't assign fields of a const object
    if type_name.starts_with(LUA_CUSTOM_CONST_PREFIX) {
        return context.push_error(
            format!("Field {qual_field_name} in a const object cannot be assigned")
        );
    }

    let mgr = ScriptManager::instance();
    let bindings = mgr.get_bindings();

    let field_res = bindings.get_field(type_name, field_name);
    if field_res.is_err() {
        return context.push_error(format!("Field {qual_field_name} is not bound"));
    }

    let field = field_res.unwrap();

    // can't assign a const field
    if field.ty.is_const {
        return context.push_error(
            format!("Field {qual_field_name} is const and cannot be assigned")
        );
    }

    // type should definitely be bound since the field is accessed through
    // its associated metatable
    let type_def = bindings.get_type_by_name(type_name)
        .expect("Failed to find bound type while setting field");

    let mut inst_wrapper = match wrap_instance_ref(&context, &qual_field_name, 1, type_def, true) {
        Ok(w) => w,
        Err(err) => { return err; }
    };

    let val_wrapper_res =
        match wrap_param(context_rc.clone(), &qual_field_name, -1, field.ty.clone()) {
            Ok(w) => w,
            Err(err) => { return context.push_error(err); }
        };

    assert!(field.assign_proxy.is_some());
    field.assign_proxy.unwrap()(&mut inst_wrapper, &val_wrapper_res);

    0
}

fn lua_clone_object(context_rc: Rc<ReentrantMutex<LuaContext>>) -> i32 {
    let context = context_rc.lock();

    let param_count = context.get_top();
    if param_count != 1 {
        return context.push_error(format!(
            "Wrong parameter count for function clone{}",
            if param_count == 0 {
                " (did you forget to use the colon operator?)"
            } else {
                ""
            }
        ));
    }

    let full_type_name = context.get_metatable_name(1).unwrap();
    let type_name = full_type_name.strip_prefix(LUA_CUSTOM_CONST_PREFIX).unwrap_or(&full_type_name);

    if !context.is_userdata(-1) {
        return context.push_error("clone() called on non-userdata object");
    }

    let mgr = ScriptManager::instance();
    let bindings = mgr.get_bindings();

    // type should definitely be bound since we're getting it directly from
    // its associated metatable

    let type_def_res = bindings.get_type_by_name(type_name);
    let type_def = type_def_res.expect("Failed to find type while cloning object");
    if type_def.cloner.is_none() {
        return context.push_error(format!("{} is not cloneable", type_name));
    }

    let udata = context.get_userdata(-1).unwrap();

    let src = match udata {
        UserDataContents::Handle(handle) =>
            context.open_handle_mut(handle, &type_def.type_id).unwrap(),
        UserDataContents::Blob(blob) =>
            blob,
    };

    let dest = context.new_userdata(type_def.size);
    let mt = context.get_metatable_by_name(&type_def.name);
    assert_ne!(mt, 0); // binding should have failed if type wasn't bound
    context.set_metatable(-2);

    type_def.cloner.unwrap()(dest.as_mut_slice().as_mut_ptr().cast(), src.as_ptr().cast());

    1
}

fn lua_type_newindex_handler(context_rc: Rc<ReentrantMutex<LuaContext>>) -> i32 {
    let context = context_rc.lock();

    let type_name = context.get_metatable_name(1).unwrap();
    let key = context.get_string(-2).unwrap();

    assert!(!type_name.is_empty());

    let res = set_native_field(&context_rc, &type_name, &key);

    res
}

fn bind_fn(context: &LuaContext, f: &BoundFunctionDef, type_name: &str) {
    // push function type
    let ty_ord: u32 = f.ty.into();
    context.push_integer(ty_ord);
    // push type name (only if member function)
    if f.ty != FunctionType::Global {
        context.push_string(type_name);
    }
    // push function name
    context.push_string(&f.name);

    let upvalue_count = if f.ty == FunctionType::Global { 2 } else { 3 };

    context.push_closure(lua_trampoline, upvalue_count);

    context.set_field(-2, &f.name);
}

fn add_type_function_to_mt(
    context: &LuaContext,
    type_name: &str,
    f: &BoundFunctionDef,
    is_const: bool
) {
    let mt_name_const = format!(
        "{}{}",
        if is_const { LUA_CUSTOM_CONST_PREFIX } else { "" },
        type_name,
    );
    context.get_metatable_by_name(mt_name_const);

    if f.ty == FunctionType::MemberInstance || f.ty == FunctionType::Extension {
        // get the dispatch table for the type
        context.get_metatable(-1);

        bind_fn(context, f, type_name);

        // pop the dispatch table and metatable
        context.pop(2);
    } else {
        bind_fn(context, f, type_name);
        // pop the metatable
        context.pop(1);
    }
}

fn bind_type_function(context: &LuaContext, type_name: &str, f: &BoundFunctionDef) {
    add_type_function_to_mt(context, type_name, f, false);
    add_type_function_to_mt(context, type_name, f, true);
}

fn bind_type_field(_context: &LuaContext, _type_name: &str, _field: &BoundFieldDef) {
    //TODO
}

fn create_type_metatable(context: &LuaContext, def: &BoundTypeDef, is_const: bool) {
    // create metatable for type
    let mt_name = if is_const {
        format!("const {}", def.name)
    } else {
        def.name.clone()
    };
    context.new_metatable(mt_name);

    // create dispatch table
    context.new_table();

    // bind __index and __newindex overrides

    // push __index function to stack
    context.push_function(lua_type_index_handler);
    // save function override
    context.set_field(-3, LUA_BUILTIN_INDEX);

    // push __newindex function to stack
    context.push_function(lua_type_newindex_handler);
    // save function override
    context.set_field(-3, LUA_BUILTIN_NEWINDEX);

    // push clone function to stack
    context.push_function(lua_clone_object);
    // save function to dispatch table
    context.set_field(-2, LUA_CUSTOM_CLONE_FN);

    // save dispatch table (which pops it from the stack)
    context.set_metatable(-2);

    if !is_const {
        // add metatable to global state to provide access to static type functions (popping it from the stack)
        context.set_global(&def.name);
    } else {
        // don't bother binding const version by name
        context.pop(1);
    }
}

fn bind_global_fn(context: &LuaContext, f: &BoundFunctionDef) {
    assert_eq!(f.ty, FunctionType::Global);

    // put the namespace table on the stack
    context.get_metatable_by_name(ENGINE_LUA_NAMESPACE);
    bind_fn(context, f, "");
    // pop the namespace table
    context.pop(1);
}

fn bind_enum(context: &LuaContext, def: &BoundEnumDef) {
    // create metatable for enum
    context.new_metatable(&def.name);

    // set values in metatable
    for (val_name, val) in &def.values {
        context.push_integer(*val);
        context.set_field(-2, val_name);
    }

    // add metatable to global state to make enum available
    context.get_metatable_by_name(&def.name);
    context.set_global(&def.name);

    // pop the metatable
    context.pop(1);
}

fn convert_path_to_uid(path: impl AsRef<str>) -> Result<String, ()> {
    let path_ref = path.as_ref();
    if path_ref.starts_with(".") || path_ref.ends_with(".") || path_ref.contains("..") {
        warn!(
            LOGGER,
            "Module name '{}' is malformed (assuming it is a resource UID)",
            path_ref,
        );
        return Err(());
    }

    if !path_ref.contains(".") {
        warn!(
            LOGGER,
            "Module name '{}' does not include a namespace (assuming it is a resource UID)",
            path_ref,
        );
        return Err(());
    }

    let spl = path_ref.split(".").collect::<Vec<&str>>();
    let ns = spl[0];
    let rel_uid = spl.into_iter().skip(1).collect::<Vec<&str>>().join("/");
    Ok(format!("{}:{}", ns, rel_uid))
}

fn load_script_src(path: impl AsRef<str>) -> Option<String> {
    if let Ok(uid) = convert_path_to_uid(path.as_ref()) {
        let mgr = ScriptManager::instance();
        if let Ok(load_res) = mgr.load_resource(LUA_LANG_NAME, uid) {
            if let Some(script) = load_res.get::<LoadedScript>() {
                return Some(script.source.clone());
            }
        }
    }

    None
}

fn require_callback(path: &str) -> Option<String> {
    load_script_src(path)
}

fn log_callback(level: lua::LogLevel, message: &str) {
    match level {
        lua::LogLevel::Trace => trace!(LOGGER, "{}", message),
        lua::LogLevel::Debug => debug!(LOGGER, "{}", message),
        lua::LogLevel::Info => info!(LOGGER, "{}", message),
        lua::LogLevel::Warning => warn!(LOGGER, "{}", message),
        lua::LogLevel::Error => severe!(LOGGER, "{}", message),
    };
}
