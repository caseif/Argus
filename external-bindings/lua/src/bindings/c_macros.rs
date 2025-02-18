use std::{ffi, ptr};
use std::ffi::{CStr, CString};
use crate::bindings::*;

// from lua.h

pub fn lua_upvalueindex(i: i32) -> i32 {
    LUA_REGISTRYINDEX - i
}

pub unsafe fn lua_call(state: *mut lua_State, nargs: i32, nresults: i32) {
    lua_callk(state, nargs, nresults, 0, None)
}

pub unsafe fn lua_pcall(state: *mut lua_State, nargs: i32, nresults: i32, errfunc: i32) -> i32 {
    lua_pcallk(state, nargs, nresults, errfunc, 0, None)
}

pub unsafe fn lua_tonumber(state: *mut lua_State, idx: i32) -> lua_Number {
    lua_tonumberx(state, idx, ptr::null_mut())
}
pub unsafe fn lua_tointeger(state: *mut lua_State, idx: i32) -> lua_Integer {
    lua_tointegerx(state, idx, ptr::null_mut())
}

pub unsafe fn lua_pop(state: *mut lua_State, idx: i32) {
    lua_settop(state, -idx - 1)
}

pub unsafe fn lua_newtable(state: *mut lua_State) {
    lua_createtable(state, 0, 0)
}

pub unsafe fn lua_register(state: *mut lua_State, name: *const ffi::c_char, f: lua_CFunction) {
    lua_pushcfunction(state, f);
    lua_setglobal(state, name)
}

pub unsafe fn lua_pushcfunction(state: *mut lua_State, f: lua_CFunction) {
    lua_pushcclosure(state, f, 0)
}

pub unsafe fn lua_isfunction(state: *mut lua_State, idx: i32) -> bool {
    lua_type(state, idx) == LUA_TFUNCTION as i32
}
pub unsafe fn lua_istable(state: *mut lua_State, idx: i32) -> bool {
    lua_type(state, idx) == LUA_TTABLE as i32
}
pub unsafe fn lua_islightuserdata(state: *mut lua_State, idx: i32) -> bool {
    lua_type(state, idx) == LUA_TLIGHTUSERDATA as i32
}
pub unsafe fn lua_isnil(state: *mut lua_State, idx: i32) -> bool {
    lua_type(state, idx) == LUA_TNIL as i32
}
pub unsafe fn lua_isboolean(state: *mut lua_State, idx: i32) -> bool {
    lua_type(state, idx) == LUA_TBOOLEAN as i32
}
pub unsafe fn lua_isthread(state: *mut lua_State, idx: i32) -> bool {
    lua_type(state, idx) == LUA_TTHREAD as i32
}
pub unsafe fn lua_isnone(state: *mut lua_State, idx: i32) -> bool {
    lua_type(state, idx) == LUA_TNONE
}
pub unsafe fn lua_isnoneornil(state: *mut lua_State, idx: i32) -> bool {
    lua_type(state, idx) <= 0
}

pub unsafe fn lua_pushglobaltable(state: *mut lua_State) -> i32 {
    lua_rawgeti(state, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS as lua_Integer)
}

pub unsafe fn lua_tostring(state: *mut lua_State, idx: i32) -> Option<String> {
    let c_str = lua_tolstring(state, idx, ptr::null_mut());
    if c_str.is_null() {
        return None;
    }
    Some(CStr::from_ptr(c_str).to_string_lossy().into_owned())
}

pub unsafe fn lua_insert(state: *mut lua_State, idx: i32) {
    lua_rotate(state, idx, 1)
}

pub unsafe fn lua_remove(state: *mut lua_State, idx: i32) {
    lua_rotate(state, idx, -1);
    lua_pop(state, 1)
}

pub unsafe fn lua_replace(state: *mut lua_State, idx: i32) {
    lua_copy(state, -1, idx);
    lua_pop(state, 1)
}

pub unsafe fn lua_newuserdata(state: *mut lua_State, sz: usize) -> *mut ffi::c_void {
    lua_newuserdatauv(state, sz, 1)
}
pub unsafe fn lua_getuservalue(state: *mut lua_State, idx: i32) -> i32 {
    lua_getiuservalue(state, idx, 1)
}
pub unsafe fn lua_setuservalue(state: *mut lua_State, idx: i32) -> i32 {
    lua_setiuservalue(state, idx, 1)
}

// from lauxlib.h

const LUAL_NUMSIZES: usize = size_of::<lua_Integer>() * 16 + size_of::<lua_Number>();

pub unsafe fn luaL_checkversion(state: *mut lua_State) {
    luaL_checkversion_(state, LUA_VERSION_NUM as lua_Number, LUAL_NUMSIZES)
}

pub unsafe fn luaL_loadfile(state: *mut lua_State, filename: *const ffi::c_char) -> i32 {
    luaL_loadfilex(state, filename, ptr::null())
}

pub unsafe fn luaL_newlibtable(state: *mut lua_State, l: &[luaL_Reg]) {
    lua_createtable(state, 0, (l.len() - 1) as i32)
}

pub unsafe fn luaL_newlib(state: *mut lua_State, l: &[luaL_Reg]) {
    luaL_checkversion(state);
    luaL_newlibtable(state, l);
    luaL_setfuncs(state, l.as_ptr(), 0)
}

pub unsafe fn luaL_checkstring(state: *mut lua_State, n: i32) -> String {
    CStr::from_ptr(luaL_checklstring(state, (n), ptr::null_mut()))
        .to_string_lossy().into_owned()
}

pub unsafe fn luaL_optstring(state: *mut lua_State, arg: i32, def: *const ffi::c_char)
    -> String {
    CStr::from_ptr(luaL_optlstring(state, arg, def, ptr::null_mut())).to_string_lossy().into_owned()
}

pub unsafe fn luaL_typename(state: *mut lua_State, idx: i32) -> String {
    CStr::from_ptr(lua_typename(state, lua_type(state, idx))).to_string_lossy().into_owned()
}

pub unsafe fn luaL_dofile(state: *mut lua_State, filename: *const ffi::c_char) -> i32 {
    let rc = luaL_loadfile(state, filename);
    if rc != 0 {
        return rc;
    }
    lua_pcall(state, 0, LUA_MULTRET, 0)
}

pub unsafe fn luaL_dostring(state: *mut lua_State, s: *const ffi::c_char) -> i32 {
    let rc = luaL_loadstring(state, s);
    if rc != 0 {
        return rc;
    }
    lua_pcall(state, 0, LUA_MULTRET, 0)
}

pub unsafe fn luaL_getmetatable(state: *mut lua_State, k: *const ffi::c_char) -> i32 {
    lua_getfield(state, LUA_REGISTRYINDEX, k)
}

pub unsafe fn luaL_loadbuffer(
    state: *mut lua_State,
    buff: *const ffi::c_char,
    sz: usize,
    n: *const ffi::c_char
) -> i32 {
    luaL_loadbufferx(state, buff, sz, n, ptr::null_mut())
}

pub unsafe fn luaL_pushfail(state: *mut lua_State) {
    lua_pushnil(state)
}
