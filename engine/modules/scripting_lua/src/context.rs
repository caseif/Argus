use crate::handles::HandleMap;
use crate::lua_language_plugin::LuaLanguagePlugin;
use crate::lua_util::create_lua_state;
use lua::bindings::{lua_State, lua_newuserdata, lua_setfield, LUA_REGISTRYINDEX};
use std::cell::RefCell;
use std::ffi::CStr;
use std::mem::MaybeUninit;
use std::rc::Rc;
use parking_lot::ReentrantMutex;

const REG_KEY_CONTEXT_DATA_PTR: &CStr = c"argus_context_data";

pub(crate) struct ManagedLuaState {
    pub(crate) state: *mut lua_State,
    pub(crate) handle_map: RefCell<HandleMap>,
}

impl From<&ManagedLuaState> for *mut lua_State {
    fn from(managed: &ManagedLuaState) -> Self {
        managed.state
    }
}

impl ManagedLuaState {
    pub(crate) fn new(plugin: &LuaLanguagePlugin) -> Rc<ReentrantMutex<Self>> {
        let raw_state = unsafe { create_lua_state(plugin) };
        let managed_state = Rc::new(ReentrantMutex::new(Self {
            state: raw_state,
            handle_map: RefCell::new(HandleMap::new()),
        }));

        unsafe {
            let udata = lua_newuserdata(
                raw_state,
                size_of::<Rc<ReentrantMutex<Self>>>(),
            );
            (*udata.cast::<MaybeUninit<Rc<ReentrantMutex<Self>>>>()).write(managed_state.clone());
            lua_setfield(
                raw_state,
                LUA_REGISTRYINDEX,
                REG_KEY_CONTEXT_DATA_PTR.as_ptr(),
            );
        }
        
        managed_state
    }
}
