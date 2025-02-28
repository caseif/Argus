use std::any::{Any, TypeId};
use std::ffi::{CStr, CString};
use std::ops::{Deref, DerefMut};
use std::ptr::{addr_of, copy_nonoverlapping};
use std::rc::Rc;
use std::sync::{Arc, Mutex};
use argus_logging::{debug, warn};
use fragile::Fragile;
use argus_scripting_bind::*;
use lua::bindings::*;
use num_enum::TryFromPrimitive;
use parking_lot::ReentrantMutex;
use resman_rs::Resource;
use scripting_rs::*;
use crate::context::ManagedLuaState;
use crate::constants::{LUA_LANG_NAME, LUA_MEDIA_TYPES};
use crate::handles::ScriptBindableHandle;
use crate::loaded_script::LoadedScript;
use crate::LOGGER;
use crate::lua_util::to_managed_state;

const ENGINE_NAMESPACE: &CStr = c"argus";

const LUA_BUILTIN_INDEX: &CStr = c"__index";
const LUA_BUILTIN_NEWINDEX: &CStr = c"__newindex";
const LUA_BUILTIN_NAME: &CStr = c"__name";
const LUA_BUILTIN_REQUIRE: &CStr = c"require";

const LUA_CUSTOM_VANILLA_REQUIRE_FN: &CStr = c"default_require";
const LUA_CUSTOM_CONST_PREFIX: &str = "const ";
const LUA_CUSTOM_CLONE_FN: &CStr = c"clone";
const LUA_CUSTOM_EMPTY_REPL: &str = "(empty)";

macro_rules! stack_guard {
    ($s:expr) => {
        {
            StackGuard::new(($s))
        }
    }
}

struct StackGuard {
    state: *mut lua_State,
    expected: i32,
}

#[allow(unused)]
impl StackGuard {
    fn new(state: impl Into<*mut lua_State>) -> Self {
        let raw_state = state.into();
        Self {
            state: raw_state,
            expected: unsafe { lua_gettop(raw_state) },
        }
    }

    fn increment_by(&mut self, count: i32) {
        self.expected += count;
    }

    fn increment(&mut self) {
        self.increment_by(1);
    }

    fn decrement_by(&mut self, count: i32) {
        assert!(count <= self.expected);
        self.increment_by(-count);
    }

    fn decrement(&mut self) {
        self.increment_by(-1);
    }
}

impl<'a> Drop for StackGuard {
    fn drop(&mut self) {
        let cur = unsafe { lua_gettop(self.state) };
        assert_eq!(self.expected, cur);
    }
}

#[repr(align(8))]
struct UserDataBlob {
    data: [u8; 0],
}

impl Deref for UserDataBlob {
    type Target = [u8; 0];

    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl DerefMut for UserDataBlob {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.data
    }
}

#[repr(C)]
struct UserData {
    is_handle: bool,
    data: UserDataBlob,
}

#[derive(Clone)]
struct LuaCallback {
    state: Fragile<*mut lua_State>,
    ref_key: i32,
}

impl ScriptCallbackRef for LuaCallback {
    fn call(&self, params: Vec<WrappedObject>)
        -> Result<WrappedObject, ScriptInvocationError> {
        let state = *self.state.get();
        let _guard = stack_guard!(state);

        unsafe {
            lua_rawgeti(state, LUA_REGISTRYINDEX, self.ref_key.into());
        }

        let state_mutex = to_managed_state(state);
        let state = state_mutex.lock();
        unsafe {
            invoke_lua_function_from_stack(state.deref(), params, None)
        }
    }
}

impl LuaCallback {
    unsafe fn new(state: &ManagedLuaState, index: i32) -> Self {
        // duplicate the top stack value in order to leave the stack as we
        // found it
        lua_pushvalue(state.state, index);
        LuaCallback {
            state: Fragile::new(state.state),
            ref_key: luaL_ref(state.state, LUA_REGISTRYINDEX),
        }
    }
}

impl Drop for LuaCallback {
    fn drop(&mut self) {
        unsafe { luaL_unref(*self.state.get(), LUA_REGISTRYINDEX, self.ref_key) };
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
        let state_mutex = ManagedLuaState::new(self);

        {
            let managed_state = state_mutex.lock();
            let raw_state = managed_state.state;

            unsafe {
                // override require behavior
                lua_getglobal(raw_state, LUA_BUILTIN_REQUIRE.as_ptr());
                lua_setglobal(raw_state, LUA_CUSTOM_VANILLA_REQUIRE_FN.as_ptr());

                lua_pushcfunction(raw_state, Some(require_override));
                lua_setglobal(raw_state, LUA_BUILTIN_REQUIRE.as_ptr());

                // create namespace table
                luaL_newmetatable(raw_state, ENGINE_NAMESPACE.as_ptr());
                lua_setglobal(raw_state, ENGINE_NAMESPACE.as_ptr());
            }
        }

        state_mutex
    }

    fn load_script(&mut self, context: &mut ScriptContext, resource: Resource)
        -> Result<(), ScriptLoadError> {
        unsafe {
            let state = context.get_plugin_data();
            load_script(state.downcast_ref::<ManagedLuaState>().unwrap(), resource).map(|_|())
        }
    }

    fn bind_type(&mut self, context: &mut ScriptContext, type_def: &BoundTypeDef) {
        unsafe {
            let state_mutex = context.get_plugin_data();
            let state = state_mutex.deref().downcast_ref::<ManagedLuaState>().unwrap();

            create_type_metatable(state, type_def, false);
            create_type_metatable(state, type_def, true);

            for field_def in type_def.fields.values() {
                bind_type_field(state, &type_def.name, &field_def);
            }

            for fn_def in type_def.static_functions.values() {
                bind_type_function(state, &type_def.name, &fn_def);
            }

            for fn_def in type_def.instance_functions.values() {
                bind_type_function(state, &type_def.name, &fn_def);
            }

            for fn_def in type_def.extension_functions.values() {
                bind_type_function(state, &type_def.name, &fn_def);
            }
        }
    }

    fn bind_global_function(&mut self, context: &mut ScriptContext, fn_def: &BoundFunctionDef) {
        let state_mutex = context.get_plugin_data();
        let state = state_mutex.deref().downcast_ref::<ManagedLuaState>().unwrap();
        unsafe { bind_global_fn(state, fn_def); }
    }

    fn bind_enum(&mut self, context: &mut ScriptContext, enum_def: &BoundEnumDef) {
        let state_mutex = context.get_plugin_data();
        let state = state_mutex.deref().downcast_ref::<ManagedLuaState>().unwrap();
        unsafe { bind_enum(state, enum_def); }
    }

    fn commit_bindings(&mut self, _context: &mut ScriptContext) {
        // no-op
    }

    fn invoke_script_function(
        &mut self,
        context: &mut ScriptContext,
        name: &str,
        params: Vec<WrappedObject>,
    ) -> Result<WrappedObject, ScriptInvocationError> {
        let plugin_data = context.get_plugin_data();
        let state: &ManagedLuaState = plugin_data.downcast_ref().unwrap();

        if params.len() > i32::MAX as usize {
            panic!("Too many params to Lua function");
        }

        let _guard = stack_guard!(state.state);

        // place Lua function on stack
        let name_c = CString::new(name).unwrap();
        unsafe { lua_getglobal(state.state, name_c.as_ptr()); }

        unsafe { invoke_lua_function_from_stack(state, params, Some(name)) }
    }
}

unsafe fn set_lua_error(state: impl Into<*mut lua_State>, msg: impl AsRef<str>) -> i32 {
    let msg_c = CString::new(msg.as_ref()).unwrap();
    luaL_error(state.into(), msg_c.as_ptr())
}

macro_rules! lua_error {
    ($context:expr, $($args:tt)*) => {
        {
            let formatted = format!($($args)*);
            set_lua_error(($context), formatted)
        }
    };
    ($context:expr, $($msg:literal)*) => {
        {
            set_lua_error(($context), literal)
        }
    };
}

unsafe fn get_metatable_name(state: impl Into<*mut lua_State>, index: i32) -> Option<String> {
    let raw_state = state.into();
    // get metatable of userdata
    lua_getmetatable(raw_state, index);

    // get metatable name
    lua_pushstring(raw_state, LUA_BUILTIN_NAME.as_ptr());
    lua_gettable(raw_state, -2);
    let type_name = lua_tostring(raw_state, -1)?;
    // remove field name and metatable from stack
    lua_pop(raw_state, 2);
    Some(type_name)
}

unsafe fn wrap_instance_ref(
    context: &ManagedLuaState,
    qual_fn_name: &str,
    param_index: i32,
    type_def: &BoundTypeDef,
    require_mut: bool
) -> Result<WrappedObject, i32> {
    if lua_isuserdata(context.state, param_index) == 0 {
        return Err(lua_error!(
            context,
            "Incorrect type provided for parameter {} of function {} (expected {}, actual {})",
            param_index,
            qual_fn_name,
            type_def.name,
            luaL_typename(context.state, param_index),
        ));
    }

    let cur_mt_name = get_metatable_name(context, param_index);

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
        return Err(lua_error!(
            context,
            "Incorrect userdata provided for parameter {} of function {} (expected {}, actual {})",
            param_index,
            qual_fn_name,
            type_def.name,
            cur_mt_name.as_ref().map(|s| s.as_str()).unwrap_or(LUA_CUSTOM_EMPTY_REPL),
        ));
    }

    let udata = &*lua_touserdata(context.state, param_index).cast::<UserData>();
    let instance_ptr = if udata.is_handle {
        let ptr = context.handle_map.borrow_mut().deref_sv_handle(
            *addr_of!(udata.data).cast::<ScriptBindableHandle>(),
            &type_def.type_id,
        );

        let Some(ptr) = ptr else {
            return Err(lua_error!(
                context,
                "Invalid handle passed as parameter {} of function {}",
                param_index,
                qual_fn_name,
            ));
        };
        ptr
    } else {
        udata.data.as_ptr().cast()
    };

    let is_const = cur_mt_name.unwrap().starts_with(LUA_CUSTOM_CONST_PREFIX);

    //TODO
    let obj_type = ObjectType {
        ty: IntegralType::Reference,
        size: size_of::<*const ()>(),
        is_const,
        is_refable: None,
        is_refable_getter: None,
        type_id: Some(type_def.type_id.clone()),
        type_name: Some(type_def.name.clone()),
        primary_type: None,
        secondary_type: None,
        callback_info: None,
        copy_ctor: None,
        move_ctor: None,
        dtor: None,
    };
    let wrapper_res = create_object_wrapper(obj_type, addr_of!(instance_ptr).cast());
    wrapper_res.map_err(|err|
        lua_error!(
            context.state,
            "Invalid arguments provided for function {}: {}",
            qual_fn_name,
            err.reason,
        )
    )
}

unsafe fn wrap_param(
    state: &ManagedLuaState,
    qual_fn_name: &str,
    param_index: i32,
    param_def: ObjectType
) -> Result<WrappedObject, String> {
    let wrapper_res = if param_def.ty.is_integral() {
        if lua_isinteger(state.state, param_index) == 0 {
            return Err(format!(
                "Incorrect type provided for parameter {} of function {} \
                     (expected integer{}, actual {})",
                param_index,
                qual_fn_name,
                if param_def.ty == IntegralType::Enum { "(enum) " } else { "" },
                luaL_typename(state.state, param_index),
            ));
        }

        create_int_object_wrapper(param_def, lua_tointeger(state.state, param_index))
    } else if param_def.ty.is_float() {
        if lua_isnumber(state.state, param_index) == 0 {
            return Err(format!(
                "Incorrect type provided for parameter {} of function {} \
                     (expected number, actual {})",
                param_index,
                qual_fn_name,
                luaL_typename(state.state, param_index),
            ));
        }

        create_float_object_wrapper(param_def, lua_tonumber(state.state, param_index))
    } else {
        match param_def.ty {
            IntegralType::Boolean => {
                if !lua_isboolean(state.state, param_index) {
                    return Err(format!(
                        "Incorrect type provided for parameter {} of function {} \
                         (expected boolean, actual {})",
                        param_index,
                        qual_fn_name,
                        luaL_typename(state.state, param_index),
                    ));
                }

                create_bool_object_wrapper(
                    param_def,
                    lua_toboolean(state.state, param_index) != 0,
                )
            }
            IntegralType::String => {
                if lua_isstring(state.state, param_index) == 0 {
                    return Err(format!(
                        "Incorrect type provided for parameter {} of function {} \
                     (expected string, actual {})",
                        param_index,
                        qual_fn_name,
                        luaL_typename(state.state, param_index),
                    ));
                }

                create_string_object_wrapper(
                    param_def,
                    &lua_tostring(state.state, param_index).unwrap()
                )
            }
            IntegralType::Object |
            IntegralType::Reference => {
                let underlying_type = if param_def.ty == IntegralType::Reference {
                    param_def.primary_type.as_ref().unwrap()
                } else {
                    &param_def
                };

                assert!(underlying_type.type_name.is_some());
                assert!(underlying_type.type_id.is_some());

                if lua_isuserdata(state.state, param_index) == 0 {
                    return Err(format!(
                        "Incorrect type provided for parameter {} of function {} \
                         (expected userdata, actual {})",
                        param_index,
                        qual_fn_name,
                        luaL_typename(state.state, param_index),
                    ));
                }

                let cur_mt_name = get_metatable_name(state, param_index);

                let expected_mt_name = param_def.type_name.as_ref().unwrap();
                let expected_mt_name_const = format!(
                    "{}{}",
                    LUA_CUSTOM_CONST_PREFIX,
                    param_def.type_name.as_ref().unwrap(),
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
                        underlying_type.type_name.as_ref().unwrap(),
                        cur_mt_name.as_ref().map(|s| s.as_str()).unwrap_or(LUA_CUSTOM_EMPTY_REPL),
                    ));
                }

                let udata = &mut *lua_touserdata(state.state, param_index).cast::<UserData>();
                let ptr = if udata.is_handle {
                    // userdata is storing handle of pointer to struct data
                    let ptr = state.handle_map.borrow_mut().deref_sv_handle(
                        *addr_of!(udata.data).cast::<ScriptBindableHandle>(),
                        underlying_type.type_id.as_ref().unwrap()
                    );

                    if ptr.is_none() {
                        return Err(format!(
                            "Invalid handle passed as parameter {} of function {}",
                            param_index,
                            qual_fn_name,
                        ));
                    }

                    ptr.unwrap()
                } else {
                    udata.data.as_mut_ptr().cast()
                };

                if param_def.ty == IntegralType::Object {
                    // pass direct pointer so that the struct data is copied
                    // into the WrappedObject
                    create_object_wrapper(param_def, ptr)
                } else {
                    // pass indirect pointer so that the pointer itself is
                    // copied into the WrappedObject
                    create_object_wrapper(param_def, addr_of!(ptr).cast())
                }
            }
            IntegralType::Callback => {
                /*if (!lua_isfunction(state, param_index)) {
                    return _set_lua_error(state, "Incorrect type provided for parameter "
                                                 + std::to_string(param_index) + " of function " + qual_fn_name
                                                 + " (expected function, actual "
                                                 + luaL_typename(state, param_index) + ")");
                }*/

                let handle = Arc::new(Mutex::new(LuaCallback::new(state, param_index)));

                let f = |
                    params: Vec<WrappedObject>,
                    data: Arc<Mutex<dyn ScriptCallbackRef>>,
                | {
                    (data.lock().unwrap()).call(params)
                };

                create_callback_object_wrapper(
                    param_def,
                    WrappedScriptCallback { entry_point: f, userdata: handle }
                )
            }
            IntegralType::Vec |
            IntegralType::VecRef => {
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

unsafe fn unwrap_int_wrapper(wrapper: WrappedObject) -> i64 {
    assert!(wrapper.ty.ty.is_integral());

    match wrapper.ty.size {
        1 => {
            *wrapper.get::<i8>() as i64
        }
        2 => {
            *wrapper.get::<i16>() as i64
        }
        4 => {
            *wrapper.get::<i32>() as i64
        }
        8 => {
            *wrapper.get::<i64>()
        }
        _ => {
            panic!("Bad integer width {} (must be 1, 2, 4, or 8)", wrapper.ty.size);
        }
    }
}

unsafe fn unwrap_float_wrapper(wrapper: WrappedObject) -> f64 {
    assert!(wrapper.ty.ty.is_float());

    match wrapper.ty.size {
        4 => {
            *wrapper.get::<f32>() as f64
        }
        8 => {
            *wrapper.get::<f64>()
        }
        _ => {
            panic!("Bad floating-point width {} (must be 4 or 8)", wrapper.ty.size);
        }
    }
}

unsafe fn unwrap_boolean_wrapper(wrapper: WrappedObject) -> bool {
    assert_eq!(wrapper.ty.ty, IntegralType::Boolean);

    *wrapper.get::<bool>()
}

unsafe fn set_metatable(state: impl Into<*mut lua_State>, ty: &ObjectType) {
    let raw_state = state.into();
    let mt_name_c = CString::new(format!(
        "{}{}",
        if ty.is_const { LUA_CUSTOM_CONST_PREFIX } else { "" },
        ty.type_name.as_ref().unwrap(),
    )).unwrap();
    let mt = luaL_getmetatable(
        raw_state,
        mt_name_c.as_ptr(),
    );
    assert_ne!(mt, 0); // binding should have failed if type wasn't bound

    lua_setmetatable(raw_state, -2);
}

unsafe fn push_value(state: &ManagedLuaState, mut wrapper: WrappedObject) {
    assert_ne!(wrapper.ty.ty, IntegralType::Empty);

    let raw_state = state.state;

    if wrapper.ty.ty.is_integral() {
        lua_pushinteger(raw_state, unwrap_int_wrapper(wrapper));
    } else if wrapper.ty.ty.is_float() {
        lua_pushnumber(raw_state, unwrap_float_wrapper(wrapper));
    } else {
        match wrapper.ty.ty {
            IntegralType::Boolean => {
                lua_pushboolean(raw_state, unwrap_boolean_wrapper(wrapper).into());
            }
            IntegralType::String => {
                lua_pushstring(raw_state, wrapper.get_ptr::<String>());
            }
            IntegralType::Object => {
                assert!(wrapper.ty.type_name.is_some());

                let udata = &mut *lua_newuserdata(
                    raw_state,
                    size_of::<UserData>() + wrapper.ty.size
                ).cast::<UserData>();
                udata.is_handle = false;
                if let Some(cloner) = wrapper.ty.copy_ctor {
                    (cloner.fn_ptr)(
                        udata.data.as_mut_ptr().cast(),
                        wrapper.get_raw_ptr(),
                    );
                } else {
                    copy_nonoverlapping(
                        wrapper.get_raw_ptr(),
                        udata.data.as_mut_ptr().cast(),
                        wrapper.ty.size,
                    );
                }
                set_metatable(raw_state, &wrapper.ty);
            }
            IntegralType::Reference => {
                assert!(wrapper.ty.type_id.is_some());
                assert!(wrapper.ty.type_name.is_some());

                let indirect_ptr: *mut *mut () = wrapper.get_mut_ptr::<&mut ()>();
                let obj_ref_ptr = *indirect_ptr;

                if !obj_ref_ptr.is_null() {
                    let state_mutex = to_managed_state(raw_state);
                    let state = state_mutex.lock();

                    let type_id = wrapper.ty.type_id.as_ref().unwrap();
                    let handle = state.handle_map.borrow_mut()
                        .get_or_create_sv_handle(type_id, obj_ref_ptr.cast());
                    let udata = &mut *lua_newuserdata(
                        raw_state,
                        size_of::<UserData>() + size_of::<ScriptBindableHandle>()
                    ).cast::<UserData>();
                    udata.is_handle = true;
                    *udata.data.as_mut_ptr().cast::<ScriptBindableHandle>() = handle;
                    set_metatable(raw_state, &wrapper.ty);
                } else {
                    lua_pushnil(raw_state);
                }
            }
            IntegralType::Vec |
            IntegralType::VecRef |
            IntegralType::Result => {
                todo!();
            }
            _ => {
                assert!(false);
            }
        }
    }
}

unsafe fn invoke_lua_function_from_stack(
    state: &ManagedLuaState,
    params: Vec<WrappedObject>,
    fn_name: Option<&str>
) -> Result<WrappedObject, ScriptInvocationError> {
    let params_count = params.len() as i32;

    for param in params {
        push_value(state, param);
    }

    if lua_pcall(state.state, params_count, 0, 0) != LUA_OK as i32 {
        let err_msg = lua_tostring(state.state, -1).unwrap();
        lua_pop(state.state, 1); // pop error message
        return Err(ScriptInvocationError {
            fn_name: fn_name.unwrap_or("callback").to_owned(),
            message: err_msg,
        });
    }

    let ty = ObjectType {
        ty: IntegralType::Empty,
        size: 0,
        is_const: false,
        is_refable: None,
        is_refable_getter: None,
        type_id: None,
        type_name: None,
        primary_type: None,
        secondary_type: None,
        callback_info: None,
        copy_ctor: None,
        move_ctor: None,
        dtor: None,
    };
    Ok(WrappedObject::new(ty, 0))
}

unsafe extern "C" fn lua_trampoline(raw_state: *mut lua_State) -> i32 {
    let mut guard = stack_guard!(raw_state);

    let state_mutex = to_managed_state(raw_state);
    let state = state_mutex.lock();

    let fn_type = FunctionType::try_from_primitive(lua_tointeger(raw_state, lua_upvalueindex(1)) as u32)
        .expect("Popped unknown function type value from Lua stack");

    let mut type_name: Option<String> = None;
    let mut fn_name_index = 2;
    if fn_type != FunctionType::Global {
        type_name = Some(lua_tostring(raw_state, lua_upvalueindex(2)).unwrap());
        fn_name_index = 3;
    }

    let fn_name = lua_tostring(raw_state, lua_upvalueindex(fn_name_index)).unwrap();

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

            return lua_error!(
                raw_state,
                "{} with name {} is not bound",
                symbol_type_disp,
                err.symbol_name,
            );
        }

        fn_res.unwrap().clone()
    };

    // parameter count not including instance
    let arg_count = lua_gettop(raw_state);
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

        return lua_error!(raw_state, "{}", err_msg);
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
            state.deref(),
            &qual_fn_name,
            param_index,
            param_def.clone(),
        );
        if let Err(err) = wrapper_res {
            return lua_error!(raw_state, "{}", err);
        }

        args.push(wrapper_res.unwrap());
    }

    let retval_res = (fn_def.proxy)(args);

    if let Err(err) = retval_res {
        return lua_error!(
            raw_state,
            "Bad arguments provided to function {} ({})",
            qual_fn_name,
            err.reason,
        );
    }

    let retval = retval_res.unwrap();

    if retval.ty.ty != IntegralType::Empty {
        push_value(state.deref(), retval);
        guard.increment();

        1
    } else {
        0
    }
}

unsafe fn lookup_fn_in_dispatch_table(context: &ManagedLuaState, mt_index: i32, key_index: i32) -> i32 {
    // get value from type's dispatch table instead
    // get type's metatable
    lua_getmetatable(context.state, mt_index);
    // get dispatch table
    lua_getmetatable(context.state, -1);
    lua_remove(context.state, -2);
    // push key onto stack
    lua_pushvalue(context.state, key_index);
    // get value of key from metatable
    lua_rawget(context.state, -2);
    lua_remove(context.state, -2);

    1
}

unsafe fn get_native_field_val(context: &ManagedLuaState, type_name: &str, field_name: &str) -> bool {
    stack_guard!(context);

    let real_type_name = if type_name.starts_with(LUA_CUSTOM_CONST_PREFIX) {
        &type_name[LUA_CUSTOM_CONST_PREFIX.len()..]
    } else {
        type_name
    };

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
        context,
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

    push_value(context, val);

    true
}

unsafe extern "C" fn lua_type_index_handler(raw_state: *mut lua_State) -> i32 {
    let mut guard = stack_guard!(raw_state);

    let state_mutex = to_managed_state(raw_state);
    let state = state_mutex.lock();

    let type_name = get_metatable_name(raw_state, 1);
    let key = lua_tostring(raw_state, -1);

    if get_native_field_val(
        state.deref(),
        &type_name.unwrap_or("".to_string()),
        &key.unwrap_or("".to_string()),
    ) {
        guard.increment();
        1
    } else {
        let retval = lookup_fn_in_dispatch_table(state.deref(), 1, 2);
        guard.increment_by(retval);
        retval
    }
}

// assumes the value is at the top of the stack
unsafe fn set_native_field(state: &ManagedLuaState, type_name: &str, field_name: &str) -> i32 {
    stack_guard!(state);

    // only necessary for the error message when the object is const since that's the only time it has the prefix
    let real_type_name = if type_name.starts_with(LUA_CUSTOM_CONST_PREFIX) {
        &type_name[LUA_CUSTOM_CONST_PREFIX.len()..]
    } else {
        type_name
    };

    let qual_field_name = get_qualified_field_name(real_type_name, field_name);

    // can't assign fields of a const object
    if type_name.starts_with(LUA_CUSTOM_CONST_PREFIX) {
        return lua_error!(state, "Field {qual_field_name} in a const object cannot be assigned");
    }

    let mgr = ScriptManager::instance();
    let bindings = mgr.get_bindings();

    let field_res = bindings.get_field(type_name, field_name);
    if field_res.is_err() {
        return lua_error!(state, "Field {qual_field_name} is not bound");
    }

    let field = field_res.unwrap();

    // can't assign a const field
    if field.ty.is_const {
        return lua_error!(state, "Field {qual_field_name} is const and cannot be assigned");
    }

    // type should definitely be bound since the field is accessed through
    // its associated metatable
    let type_def = bindings.get_type_by_name(type_name)
        .expect("Failed to find bound type while setting field");

    let mut inst_wrapper = match wrap_instance_ref(state, &qual_field_name, 1, type_def, true) {
        Ok(w) => w,
        Err(err) => { return err; }
    };

    let val_wrapper_res = match wrap_param(state, &qual_field_name, -1, field.ty.clone()) {
        Ok(w) => w,
        Err(err) => { return lua_error!(state, "{}", err); }
    };

    assert!(field.assign_proxy.is_some());
    field.assign_proxy.unwrap()(&mut inst_wrapper, &val_wrapper_res);

    0
}

unsafe extern "C" fn lua_type_newindex_handler(raw_state: *mut lua_State) -> i32 {
    stack_guard!(raw_state);

    let state_mutex = to_managed_state(raw_state);
    let state = state_mutex.lock();

    let type_name = get_metatable_name(raw_state, 1).unwrap();
    let key = lua_tostring(raw_state, -2).unwrap();

    assert!(type_name.len() > 0);

    let res = set_native_field(state.deref(), &type_name, &key);

    res
}

unsafe extern "C" fn lua_clone_object(raw_state: *mut lua_State) -> i32 {
    let mut guard = stack_guard!(raw_state);

    let state_mutex = to_managed_state(raw_state);
    let state = state_mutex.lock();

    let param_count = lua_gettop(raw_state);
    if param_count != 1 {
        return lua_error!(
            raw_state,
            "Wrong parameter count for function clone{}",
            if param_count == 0 {
                " (did you forget to use the colon operator?)"
            } else {
                ""
            }
        );
    }

    let full_type_name = get_metatable_name(raw_state, 1).unwrap();
    let type_name = if full_type_name.starts_with(LUA_CUSTOM_CONST_PREFIX) {
        &full_type_name[LUA_CUSTOM_CONST_PREFIX.len()..]
    } else {
        &full_type_name
    };

    if lua_isuserdata(raw_state, -1) == 0 {
        return lua_error!(raw_state, "clone() called on non-userdata object");
    }

    let mgr = ScriptManager::instance();
    let bindings = mgr.get_bindings();

    // type should definitely be bound since we're getting it directly from
    // its associated metatable

    let type_def_res = bindings.get_type_by_name(type_name);
    let type_def = type_def_res.expect("Failed to find type while cloning object");
    if type_def.cloner.is_none() {
        return lua_error!(raw_state, "{} is not cloneable", type_name);
    }

    let udata = &mut *lua_touserdata(raw_state, -1).cast::<UserData>();

    let src = if udata.is_handle {
        let handle = udata.data.as_ptr().cast::<ScriptBindableHandle>();
        state.handle_map.borrow_mut().deref_sv_handle(*handle, &type_def.type_id)
    } else {
        Some(udata.data.as_mut_ptr().cast())
    };

    let dest = &mut *lua_newuserdata(
        raw_state,
        size_of::<UserData>() + type_def.size
    ).cast::<UserData>();
    dest.is_handle = false;
    guard.increment();
    let type_name_c = CString::new(type_def.name.as_str()).unwrap();
    let mt = luaL_getmetatable(raw_state, type_name_c.as_ptr());
    assert_ne!(mt, 0); // binding should have failed if type wasn't bound
    lua_setmetatable(raw_state, -2);

    type_def.cloner.unwrap()(dest.data.as_mut_ptr().cast(), src.unwrap());

    1
}

unsafe fn bind_fn(context: &ManagedLuaState, f: &BoundFunctionDef, type_name: &str) {
    // push function type
    let ty_ord: u32 = f.ty.into();
    lua_pushinteger(context.state, ty_ord as lua_Integer);
    // push type name (only if member function)
    if f.ty != FunctionType::Global {
        let type_name_c = CString::new(type_name).unwrap();
        lua_pushstring(context.state, type_name_c.as_ptr());
    }
    // push function name
    let name_c = CString::new(f.name.as_str()).unwrap();
    lua_pushstring(context.state, name_c.as_ptr());

    let upvalue_count = if f.ty == FunctionType::Global { 2 } else { 3 };

    lua_pushcclosure(context.state, Some(lua_trampoline), upvalue_count);

    lua_setfield(context.state, -2, name_c.as_ptr());
}

unsafe fn add_type_function_to_mt(
    context: &ManagedLuaState,
    type_name: &str,
    f: &BoundFunctionDef,
    is_const: bool
) {
    let mt_name_const = CString::new(format!(
        "{}{}",
        if is_const { LUA_CUSTOM_CONST_PREFIX } else { "" },
        type_name,
    )).unwrap();
    luaL_getmetatable(context.state, mt_name_const.as_ptr());

    if f.ty == FunctionType::MemberInstance || f.ty == FunctionType::Extension {
        // get the dispatch table for the type
        lua_getmetatable(context.state, -1);

        bind_fn(context, f, type_name);

        // pop the dispatch table and metatable
        lua_pop(context.state, 2);
    } else {
        bind_fn(context, f, type_name);
        // pop the metatable
        lua_pop(context.state, 1);
    }
}

unsafe fn bind_type_function(context: &ManagedLuaState, type_name: &str, f: &BoundFunctionDef) {
    add_type_function_to_mt(context, type_name, f, false);
    add_type_function_to_mt(context, type_name, f, true);
}

unsafe fn bind_type_field(_context: &ManagedLuaState, _type_name: &str, _field: &BoundFieldDef) {
    //TODO
}

unsafe fn create_type_metatable(lua: &ManagedLuaState, def: &BoundTypeDef, is_const: bool) {
    // create metatable for type
    let mt_name_c = CString::new(if is_const {
        format!("const {}", def.name)
    } else {
        def.name.clone()
    }).unwrap();
    luaL_newmetatable(
        lua.state,
        mt_name_c.as_ptr(),
    );

    // create dispatch table
    lua_newtable(lua.state);

    // bind __index and __newindex overrides

    // push __index function to stack
    lua_pushcfunction(lua.state, Some(lua_type_index_handler));
    // save function override
    lua_setfield(lua.state, -3, LUA_BUILTIN_INDEX.as_ptr());

    // push __newindex function to stack
    lua_pushcfunction(lua.state, Some(lua_type_newindex_handler));
    // save function override
    lua_setfield(lua.state, -3, LUA_BUILTIN_NEWINDEX.as_ptr());

    // push clone function to stack
    lua_pushcfunction(lua.state, Some(lua_clone_object));
    // save function to dispatch table
    lua_setfield(lua.state, -2, LUA_CUSTOM_CLONE_FN.as_ptr());

    // save dispatch table (which pops it from the stack)
    lua_setmetatable(lua.state, -2);

    if !is_const {
        // add metatable to global state to provide access to static type functions (popping it from the stack)
        let name_c = CString::new(def.name.as_str()).unwrap();
        lua_setglobal(lua.state, name_c.as_ptr());
    } else {
        // don't bother binding const version by name
        lua_pop(lua.state, 1);
    }
}

unsafe fn bind_type(context: &ManagedLuaState, ty: BoundTypeDef) {
    create_type_metatable(context, &ty, false);
    create_type_metatable(context, &ty, true);

    for field_def in ty.fields.values() {
        bind_type_field(context, &ty.name, field_def);
    }

    for fn_def in ty.static_functions.values() {
        bind_type_function(context, &ty.name, fn_def);
    }

    for fn_def in ty.instance_functions.values() {
        bind_type_function(context, &ty.name, fn_def);
    }

    for fn_def in ty.extension_functions.values() {
        bind_type_function(context, &ty.name, fn_def);
    }
}

unsafe fn bind_global_fn(context: &ManagedLuaState, f: &BoundFunctionDef) {
    assert_eq!(f.ty, FunctionType::Global);

    // put the namespace table on the stack
    luaL_getmetatable(context.state, ENGINE_NAMESPACE.as_ptr());
    bind_fn(context, f, "");
    // pop the namespace table
    lua_pop(context.state, 1);
}

unsafe fn bind_enum(context: &ManagedLuaState, def: &BoundEnumDef) {
    let name_c = CString::new(def.name.as_str()).unwrap();

    // create metatable for enum
    luaL_newmetatable(context.state, name_c.as_ptr());

    // set values in metatable
    for (val_name, val) in &def.values {
        let val_name_c = CString::new(val_name.as_str()).unwrap();
        lua_pushinteger(context.state, *val);
        lua_setfield(context.state, -2, val_name_c.as_ptr());
    }

    // add metatable to global state to make enum available
    luaL_getmetatable(context.state, name_c.as_ptr());
    lua_setglobal(context.state, name_c.as_ptr());

    // pop the metatable
    lua_pop(context.state, 1);
}

unsafe fn convert_path_to_uid(path: &str) -> Result<String, ()> {
    if path.starts_with(".") || path.ends_with(".") || path.contains("..") {
        warn!(
            LOGGER,
            "Module name '{}' is malformed (assuming it is a resource UID)",
            path,
        );
        return Err(());
    }

    if !path.contains(".") {
        warn!(
            LOGGER,
            "Module name '{}' does not include a namespace (assuming it is a resource UID)",
            path,
        );
        return Err(());
    }

    let spl = path.split(".").collect::<Vec<&str>>();
    let ns = spl[0];
    let rel_uid = spl.into_iter().skip(1).collect::<Vec<&str>>().join("/");
    Ok(format!("{}:{}", ns, rel_uid))
}

unsafe fn load_script(state: impl Into<*mut lua_State>, resource: Resource)
    -> Result<i32, ScriptLoadError> {
    let state = state.into();
    let loaded_script = resource.get::<LoadedScript>().unwrap();

    let src_c = CString::new(loaded_script.source.as_str()).unwrap();
    let uid = resource.get_prototype().uid.to_string();
    let uid_c = CString::new(uid.as_str()).unwrap();
    let load_res = luaL_loadbuffer(
        state,
        src_c.as_ptr(),
        loaded_script.source.len(),
        uid_c.as_ptr(),
    );
    if load_res != LUA_OK as i32 {
        let err_msg = lua_tostring(state, -1).unwrap();
        return Err(ScriptLoadError::new(
            uid,
            format!("Failed to parse script: {}", err_msg),
        ));
    }

    let call_res = lua_pcall(state, 0, 1, 0);
    if call_res != LUA_OK as i32 {
        //TODO: print detailed trace info from VM
        return Err(ScriptLoadError::new(uid, lua_tostring(state, -1).unwrap()));
    }

    Ok(1)
}

unsafe extern "C" fn require_override(state: *mut lua_State) -> i32 {
    let Some(path) = lua_tostring(state, 1) else {
        return lua_error!(state, "Incorrect arguments to function 'require'");
    };

    if let Ok(uid) = convert_path_to_uid(&path) {
        let mut mgr = ScriptManager::instance();
        let load_res = mgr.load_resource(LUA_LANG_NAME, uid);
        if load_res.is_ok() {
            let load_script_res = load_script(state, load_res.unwrap());
            return match load_script_res {
                Ok(v) => v,
                Err(err) => lua_error!(
                    state,
                    "Unable to parse script {} passed to 'require': {}",
                    path,
                    err.msg
                )
            };
        } else {
            debug!(
                LOGGER,
                "Unable to load resource for require path {} ({})",
                path,
                load_res.unwrap_err().msg,
            );
            // swallow
        }
    }

    warn!(
        LOGGER,
        "Unable to load Lua module '{}' as resource; falling back to default require behavior",
        path,
    );

    // If load_script failed, fall back to old require
    lua_getglobal(state, LUA_CUSTOM_VANILLA_REQUIRE_FN.as_ptr());
    let path_c = CString::new(path.as_str()).unwrap();
    lua_pushstring(state, path_c.as_ptr());
    if lua_pcall(state, 0, 1, 0) != 0 {
        return lua_error!(
            state,
            "Error executing function 'require': {}",
            lua_tostring(state, -1).unwrap(),
        );
    }

    1
}
