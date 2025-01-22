#include "argus/scripting/manager.hpp"
#include "argus/scripting/cabi/manager.h"

static argus::ScriptManager &_mgr_as_ref(argus_script_manager_t mgr) {
    return *reinterpret_cast<argus::ScriptManager *>(mgr);
}

static argus::BoundTypeDef &_type_def_as_ref(argus_bound_type_def_t def) {
    return *reinterpret_cast<argus::BoundTypeDef *>(def);
}

static argus::BoundEnumDef &_enum_def_as_ref(argus_bound_enum_def_t def) {
    return *reinterpret_cast<argus::BoundEnumDef *>(def);
}

static argus::BoundFunctionDef &_fn_def_as_ref(argus_bound_function_def_t def) {
    return *reinterpret_cast<argus::BoundFunctionDef *>(def);
}

// ALLOCATES
static ArgusMaybeBindingError _wrap_res(const argus::Result<void, argus::BindingError> &res) {
    if (res.is_err()) {
        return { true, new argus::BindingError(res.unwrap_err()) };
    } else {
        return { false, nullptr };
    }
}

[[maybe_unused]] static argus::Result<void, argus::BindingError> _unwrap_res(const ArgusMaybeBindingError &res) {
    if (res.is_err) {
        return argus::err<void, argus::BindingError>(*reinterpret_cast<argus::BindingError *>(res.error));
    } else {
        return argus::ok<void, argus::BindingError>();
    }
}

extern "C" {

argus_script_manager_t argus_script_manager_instance() {
    return &argus::ScriptManager::instance();
}

ArgusMaybeBindingError argus_script_manager_bind_type(argus_script_manager_t manager, argus_bound_type_def_t def) {
    auto def_ref = _type_def_as_ref(def);
    auto res = _mgr_as_ref(manager).bind_type(def_ref);
    return _wrap_res(res);
}

ArgusMaybeBindingError argus_script_manager_bind_enum(argus_script_manager_t manager, argus_bound_enum_def_t def) {
    auto def_ref = _enum_def_as_ref(def);
    auto res = _mgr_as_ref(manager).bind_enum(def_ref);
    return _wrap_res(res);
}

ArgusMaybeBindingError argus_script_manager_bind_global_function(argus_script_manager_t manager,
        argus_bound_function_def_t def) {
    auto def_ref = _fn_def_as_ref(def);
    auto res = _mgr_as_ref(manager).bind_global_function(def_ref);
    return _wrap_res(res);
}

}
