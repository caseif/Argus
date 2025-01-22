#pragma once

#include "argus/scripting/cabi/bind.h"
#include "argus/scripting/cabi/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *argus_script_manager_t;
typedef const void *argus_script_manager_const_t;

argus_script_manager_t argus_script_manager_instance();

typedef argus_object_wrapper_t (*ArgusFieldAccessor)(argus_object_wrapper_const_t inst, argus_object_type_const_t,
        const void *state);
typedef void (*ArgusFieldMutator)(argus_object_wrapper_t inst, argus_object_wrapper_t, const void *state);

ArgusMaybeBindingError argus_script_manager_bind_type(argus_script_manager_t manager, argus_bound_type_def_t def);

ArgusMaybeBindingError argus_script_manager_bind_enum(argus_script_manager_t manager, argus_bound_enum_def_t def);

ArgusMaybeBindingError argus_script_manager_bind_global_function(argus_script_manager_t manager,
        argus_bound_function_def_t def);

#ifdef __cplusplus
}
#endif
