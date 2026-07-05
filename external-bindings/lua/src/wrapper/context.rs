use std::cell::RefCell;
use std::{ffi, slice};
use std::ffi::{CStr, CString};
use std::mem::MaybeUninit;
use std::ops::{Deref, DerefMut};
use std::rc::Rc;
use std::str::FromStr;
use fragile::Fragile;
use num_enum::{IntoPrimitive, TryFromPrimitive};
use num_traits::{Float, PrimInt};
use parking_lot::ReentrantMutex;
use crate::bindings::{luaL_error, luaL_getmetatable, luaL_loadbuffer, luaL_newmetatable, luaL_newstate, luaL_openlibs, luaL_ref, luaL_typename, luaL_unref, lua_State, lua_close, lua_getfield, lua_getglobal, lua_getmetatable, lua_gettable, lua_gettop, lua_isboolean, lua_isinteger, lua_isnumber, lua_isstring, lua_isuserdata, lua_newtable, lua_newuserdata, lua_pcall, lua_pop, lua_pushboolean, lua_pushcclosure, lua_pushcfunction, lua_pushinteger, lua_pushnil, lua_pushnumber, lua_pushstring, lua_pushvalue, lua_rawget, lua_rawgeti, lua_remove, lua_setfield, lua_setglobal, lua_setmetatable, lua_toboolean, lua_tointeger, lua_tonumber, lua_tostring, lua_touserdata, LUA_OK, LUA_REGISTRYINDEX};
use crate::log;
use crate::wrapper::handle::{FfiHandle, FfiHandleMap};
use crate::wrapper::load::ScriptLoadError;
use crate::wrapper::types::LogLevel;

pub use crate::bindings::lua_upvalueindex;

const LUA_BUILTIN_NAME: &str = "__name";
const LUA_BUILTIN_REQUIRE: &str = "require";

const LUA_CUSTOM_VANILLA_REQUIRE_FN: &str = "default_require";

const REG_KEY_CONTEXT_DATA_PTR: &CStr = c"__lua_context_data";

#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, Ord, PartialEq, PartialOrd, TryFromPrimitive)]
#[repr(i32)]
pub enum LuaRetval {
    Ok = 0,
    Yield = 1,
    ErrRun = 2,
    ErrSyntax = 3,
    ErrMem = 4,
    ErrErr = 5,
}

#[macro_export]
macro_rules! upvalue {
    ($i:expr) => {
        $crate::wrapper::context::lua_upvalueindex($i)
    }
}

pub struct StackGuard {
    state: *mut lua_State,
    expected: i32,
}

#[allow(unused)]
impl StackGuard {
    #[must_use]
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

impl Drop for StackGuard {
    fn drop(&mut self) {
        let cur = unsafe { lua_gettop(self.state) };
        assert_eq!(self.expected, cur);
    }
}

macro_rules! stack_guard {
    ($s:expr) => {
        {
            StackGuard::new(($s))
        }
    }
}

unsafe extern "C" fn cfunction_trampoline<F: Fn(Rc<ReentrantMutex<LuaContext>>) -> i32>(state: *mut lua_State) -> i32 {
    let f: F = unsafe { std::mem::zeroed::<F>() };
    let context_rc = get_context(state);
    let context = context_rc.lock();

    let mut guard = stack_guard!(context.state);
    let rv = f(context_rc.clone());
    guard.increment_by(rv);

    rv
}

pub struct LuaRef {
    pub(crate) context: Fragile<Rc<ReentrantMutex<LuaContext>>>,
    pub(crate) ref_index: i32,
}

impl LuaRef {
    pub fn push_to_stack(&self) {
        let context = self.context.get().lock();
        unsafe { lua_rawgeti(context.state, LUA_REGISTRYINDEX, self.ref_index.into()); }
    }
}

impl Drop for LuaRef {
    fn drop(&mut self) {
        let context = self.context.get().lock();
        context.drop_ref(self.ref_index);
    }
}

#[repr(align(8))]
pub struct UserDataBlob {
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

pub enum UserDataContents<'a> {
    Blob(&'a [u8]),
    Handle(FfiHandle),
}

pub enum UserDataContentsMut<'a> {
    Blob(&'a mut [u8]),
    Handle(FfiHandle),
}

#[repr(C)]
pub struct UserData {
    is_handle: bool,
    size: usize,
    data: UserDataBlob,
}

impl UserData {
    pub fn as_slice(&self) -> &[u8] {
        unsafe { slice::from_raw_parts(self.data.as_ptr(), self.size) }
    }

    pub fn as_mut_slice(&mut self) -> &mut [u8] {
        unsafe { slice::from_raw_parts_mut(self.data.as_mut_ptr(), self.size) }
    }
}

pub struct LuaContext {
    pub(crate) state: *mut lua_State,
    pub(crate) ns: String,
    pub(crate) handle_map: RefCell<FfiHandleMap>,
    log_callback: Option<fn(LogLevel, &str) -> ()>,
    require_callback: Option<fn(&str) -> Option<String>>,
}

impl LuaContext {
    pub fn create(
        ns: impl Into<String>,
        require_callback: Option<fn(path: &str) -> Option<String>>,
        log_callback: Option<fn(LogLevel, &str) -> ()>,
    ) -> Rc<ReentrantMutex<Self>> {
        let raw_state = create_lua_state();
        let context_rc = Rc::new(ReentrantMutex::new(Self {
            state: raw_state,
            ns: ns.into(),
            handle_map: RefCell::new(FfiHandleMap::new()),
            log_callback,
            require_callback,
        }));

        {
            let context = context_rc.lock();

            unsafe {
                let udata = lua_newuserdata(
                    raw_state,
                    size_of::<Rc<ReentrantMutex<Self>>>(),
                );
                (*udata.cast::<MaybeUninit<Rc<ReentrantMutex<Self>>>>()).write(context_rc.clone());
            }

            context.set_field(
                LUA_REGISTRYINDEX,
                REG_KEY_CONTEXT_DATA_PTR.to_string_lossy(),
            );

            // override require behavior
            context.get_global(LUA_BUILTIN_REQUIRE);
            context.set_global(LUA_CUSTOM_VANILLA_REQUIRE_FN);

            context.push_function(require_override);
            context.set_global(LUA_BUILTIN_REQUIRE);

            // create namespace table
            context.new_metatable(&context.ns);
            context.set_global(&context.ns);
        }

        context_rc
    }

    pub fn get_stack_guard(&self) -> StackGuard {
        stack_guard!(self.state)
    }

    pub fn load_script(&self, path: impl AsRef<str>, src: impl AsRef<str>)
                                  -> Result<i32, ScriptLoadError> {
        let path_ref = path.as_ref();
        let src_ref = src.as_ref();
        let src_c = CString::new(src_ref).unwrap();
        let path_c = CString::new(path_ref).unwrap();
        let load_res = unsafe {
            luaL_loadbuffer(
                self.state,
                src_c.as_ptr(),
                src_ref.len(),
                path_c.as_ptr(),
            )
        };
        if load_res != LUA_OK as i32 {
            let err_msg = unsafe { lua_tostring(self.state, -1) }.unwrap();
            return Err(ScriptLoadError::new(
                path.as_ref(),
                format!("Failed to parse script: {}", err_msg),
            ));
        }

        let call_res = unsafe { lua_pcall(self.state, 0, 1, 0) };
        if call_res != LUA_OK as i32 {
            //TODO: print detailed trace info from VM
            return Err(ScriptLoadError::new(
                path_ref,
                unsafe { lua_tostring(self.state, -1) }.unwrap(),
            ));
        }

        Ok(1)
    }

    pub fn push_error(&self, msg: impl Into<String>) -> i32 {
        let msg_c = CString::new(msg.into()).unwrap();
        unsafe { luaL_error(self.state, msg_c.as_ptr()) }
    }

    pub fn get_top(&self) -> i32 {
        unsafe { lua_gettop(self.state) }
    }

    pub fn get_global(&self, name: impl AsRef<str>) {
        let name_c = CString::new(name.as_ref()).unwrap();
        unsafe { lua_getglobal(self.state, name_c.as_ptr()); }
    }

    pub fn set_global(&self, name: impl AsRef<str>) {
        let name_c = CString::new(name.as_ref()).unwrap();
        unsafe { lua_setglobal(self.state, name_c.as_ptr()); }
    }

    pub fn new_table(&self) {
        unsafe { lua_newtable(self.state); }
    }

    pub fn get_metatable(&self, index: i32) {
        unsafe { lua_getmetatable(self.state, index); }
    }

    pub fn get_metatable_by_name(&self, name: impl AsRef<str>) -> i32 {
        let name_c = CString::new(name.as_ref()).unwrap();
        unsafe { luaL_getmetatable(self.state, name_c.as_ptr()) }
    }

    pub fn get_metatable_name(&self, index: i32) -> Option<String> {
        let raw_state = self.state;
        // get metatable of userdata
        unsafe { lua_getmetatable(raw_state, index); }

        // get metatable name
        self.push_string(LUA_BUILTIN_NAME);
        unsafe { lua_gettable(raw_state, -2); }
        let type_name = unsafe { lua_tostring(raw_state, -1)? };
        // remove field name and metatable from stack
        unsafe { lua_pop(raw_state, 2); }
        Some(type_name)
    }

    pub fn set_metatable(&self, obj_index: i32) {
        unsafe { lua_setmetatable(self.state, obj_index); }
    }

    pub fn new_metatable(&self, name: impl AsRef<str>) -> i32 {
        let name_c = CString::new(name.as_ref()).unwrap();
        unsafe { luaL_newmetatable(self.state, name_c.as_ptr()) }
    }

    pub fn set_field(&self, index: i32, name: impl AsRef<str>) {
        let name_c = CString::from_str(name.as_ref()).unwrap();
        unsafe { lua_setfield(self.state, index, name_c.as_ptr()); }
    }

    pub fn get_typename(&self, index: i32) -> String {
        unsafe { luaL_typename(self.state, index) }
    }

    pub fn push(&self, index: i32) {
        unsafe { lua_pushvalue(self.state, index); }
    }

    pub fn pop(&self, index: i32) {
        unsafe { lua_pop(self.state, index) };
    }

    pub fn raw_get(&self, index: i32) {
        unsafe { lua_rawget(self.state, index); }
    }

    pub fn remove(&self, index: i32) {
        unsafe { lua_remove(self.state, index); }
    }

    pub fn is_integer(&self, index: i32) -> bool {
        unsafe { lua_isinteger(self.state, index) != 0 }
    }

    pub fn is_number(&self, index: i32) -> bool {
        unsafe { lua_isnumber(self.state, index) != 0 }
    }

    pub fn is_boolean(&self, index: i32) -> bool {
        unsafe { lua_isboolean(self.state, index) }
    }

    pub fn is_string(&self, index: i32) -> bool {
        unsafe { lua_isstring(self.state, index) != 0 }
    }

    pub fn is_userdata(&self, index: i32) -> bool {
        unsafe { lua_isuserdata(self.state, index) != 0 }
    }

    pub fn get_integer<T: PrimInt>(&self, index: i32) -> T {
        T::from(unsafe { lua_tointeger(self.state, index) }).unwrap()
    }

    pub fn get_number<T: Float>(&self, index: i32) -> T {
        T::from(unsafe { lua_tonumber(self.state, index) }).unwrap()
    }

    pub fn get_boolean(&self, index: i32) -> bool {
        unsafe { lua_toboolean(self.state, index) != 0 }
    }

    pub fn get_string(&self, index: i32) -> Option<String> {
        unsafe { lua_tostring(self.state, index) }
    }

    pub fn push_nil(&self) {
        unsafe { lua_pushnil(self.state); }
    }

    pub fn push_integer(&self, value: impl PrimInt) {
        unsafe { lua_pushinteger(self.state, value.to_i64().unwrap()); }
    }

    pub fn push_number(&self, value: impl Float) {
        unsafe { lua_pushnumber(self.state, value.to_f64().unwrap()); }
    }

    pub fn push_boolean(&self, value: bool) {
        unsafe { lua_pushboolean(self.state, value as ffi::c_int); }
    }

    pub fn push_string(&self, value: impl AsRef<str>) {
        let value_c = CString::new(value.as_ref()).unwrap();
        unsafe { lua_pushstring(self.state, value_c.as_ptr()); }
    }

    pub fn push_function<F: Fn(Rc<ReentrantMutex<LuaContext>>) -> i32>(&self, _f: F) {
        unsafe { lua_pushcfunction(self.state, Some(cfunction_trampoline::<F>)); }
    }

    pub fn push_closure<F: Fn(Rc<ReentrantMutex<LuaContext>>) -> i32>(&self, _f: F, n: i32) {
        unsafe { lua_pushcclosure(self.state, Some(cfunction_trampoline::<F>), n); }
    }

    pub fn pcall(&self, nargs: i32, nresults: i32, errfunc: i32) -> LuaRetval {
        unsafe {
            LuaRetval::try_from_primitive(lua_pcall(self.state, nargs, nresults, errfunc)).unwrap()
        }
    }

    pub fn get_userdata(&'_ self, index: i32) -> Option<UserDataContents<'_>> {
        let udata = unsafe {
            let udata_p = lua_touserdata(self.state, index).cast::<UserData>();
            if udata_p.is_null() {
                return None;
            }
            udata_p.as_ref().unwrap()
        };

        if udata.is_handle {
            Some(UserDataContents::Handle(unsafe { *udata.data.as_ptr().cast::<FfiHandle>() }))
        } else {
            let blob = unsafe { slice::from_raw_parts(udata.data.data.as_ptr(), udata.size) };
            Some(UserDataContents::Blob(blob))
        }
    }

    pub fn get_userdata_mut(&'_ self, index: i32) -> Option<UserDataContentsMut<'_>> {
        let udata = unsafe {
            let udata_p = lua_touserdata(self.state, index).cast::<UserData>();
            if udata_p.is_null() {
                return None;
            }
            udata_p.as_mut().unwrap()
        };

        if udata.is_handle {
            Some(UserDataContentsMut::Handle(
                unsafe { *udata.data.as_ptr().cast::<FfiHandle>() }
            ))
        } else {
            let blob = unsafe {
                slice::from_raw_parts_mut(udata.data.data.as_mut_ptr(), udata.size)
            };
            Some(UserDataContentsMut::Blob(blob))
        }
    }

    pub fn open_handle(&self, handle: FfiHandle, expected_type_id: impl AsRef<str>)
        -> Option<&[u8]> {
        self.handle_map.borrow_mut().deref_sv_handle(
            handle,
            expected_type_id.as_ref(),
        ).and_then(|(size, ptr)| {
            unsafe { Some(slice::from_raw_parts(ptr.cast(), size)) }
        })
    }

    pub fn open_handle_mut(&self, handle: FfiHandle, expected_type_id: impl AsRef<str>)
                       -> Option<&mut [u8]> {
        self.handle_map.borrow_mut().deref_sv_handle(
            handle,
            expected_type_id.as_ref(),
        ).and_then(|(size, ptr)| {
            unsafe { Some(slice::from_raw_parts_mut(ptr.cast(), size)) }
        })
    }

    pub fn new_userdata(&self, size: usize) -> &mut UserData {
        let udata = unsafe {
            &mut *lua_newuserdata(
                self.state,
                size_of::<UserData>() + size
            ).cast::<UserData>()
        };
        udata.is_handle = false;
        udata.size = size;
        udata
    }

    pub fn new_handle(&self, type_id: impl AsRef<str>, size: usize, ptr: *mut ()) -> FfiHandle {
        let handle = self.handle_map.borrow_mut()
            .get_or_create_sv_handle(type_id.as_ref(), size, ptr.cast());

        let udata = unsafe {
            &mut *lua_newuserdata(
                self.state,
                size_of::<UserData>() + size_of::<FfiHandle>()
            ).cast::<UserData>()
        };
        udata.is_handle = true;
        udata.size = size;
        unsafe { *udata.data.as_mut_ptr().cast::<FfiHandle>() = handle; }

        handle
    }

    pub fn create_ref(rc: &Rc<ReentrantMutex<Self>>, index: i32) -> LuaRef {
        let context = rc.lock();
        unsafe { lua_pushvalue(context.state, index); }
        let ref_index = unsafe { luaL_ref(context.state, LUA_REGISTRYINDEX) };
        LuaRef {
            context: Fragile::new(rc.clone()),
            ref_index,
        }
    }

    fn drop_ref(&self, ref_index: i32) {
        unsafe { luaL_unref(self.state, LUA_REGISTRYINDEX, ref_index); }
    }
}

fn create_lua_state() -> *mut lua_State {
    let state = unsafe { luaL_newstate() };
    if state.is_null() {
        panic!("Failed to create Lua state");
    }

    unsafe { luaL_openlibs(state) };

    state
}

#[allow(dead_code)]
fn destroy_lua_state(state: *mut lua_State) {
    unsafe { lua_close(state); }
}

pub(crate) fn get_context(state: *mut lua_State) -> Rc<ReentrantMutex<LuaContext>> {
    unsafe {
        lua_getfield(state, LUA_REGISTRYINDEX, REG_KEY_CONTEXT_DATA_PTR.as_ptr());
        let ret_val =
            (*lua_touserdata(state, -1).cast::<Rc<ReentrantMutex<LuaContext>>>()).clone();
        lua_pop(state, 1);
        ret_val
    }
}

fn require_override(context_rc: Rc<ReentrantMutex<LuaContext>>) -> i32 {
    let context = context_rc.lock();
    let Some(path) = context.get_string(1) else {
        return context.push_error("Incorrect arguments to function 'require'");
    };

    if let Some(callback) = context.require_callback {
        let load_res = callback(path.as_str());
        match load_res {
            Some(loaded_src) => {
                return match context.load_script(path.as_str(), loaded_src.as_str()) {
                    Ok(v) => v,
                    Err(err) => context.push_error(format!(
                        "Unable to parse script {} passed to 'require': {}",
                        path,
                        err.message,
                    )),
                }
            }
            None => {
                log!(
                    context,
                    LogLevel::Debug,
                    "Unable to load resource for require path {}",
                    path,
                );
            }
        }
    }

    log!(
        context,
        LogLevel::Warning,
        "Unable to load Lua module '{}' via callback; falling back to default require behavior",
        path,
    );

    // If load_script failed, fall back to old require
    context.get_global(LUA_CUSTOM_VANILLA_REQUIRE_FN);
    context.push_string(path);
    if context.pcall(0, 1, 0) != LuaRetval::Ok {
        return context.push_error(format!(
            "Error executing function 'require': {}",
            context.get_string(-1).unwrap(),
        ));
    }

    1
}
