/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/result.hpp"

#include "argus/core/engine.hpp"

#include "argus/resman.hpp"

#include "argus/scripting/error.hpp"
#include "argus/scripting/handles.hpp"
#include "argus/scripting/manager.hpp"
#include "argus/scripting/util.hpp"
#include "argus/scripting/wrapper.hpp"

#include "internal/scripting_lua/context_data.hpp"
#include "internal/scripting_lua/defines.hpp"
#include "internal/scripting_lua/loaded_script.hpp"
#include "internal/scripting_lua/lua_language_plugin.hpp"

#include "internal/scripting_lua/module_scripting_lua.hpp"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include <functional>
#include <memory>
#include <string>

namespace argus {
    const std::vector<std::string> k_plugin_resource_types = { k_resource_type_lua };

    // disable non-standard extension warning for zero-sized array member
    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4200)
    #endif
    struct UserData {
        bool is_handle;
        alignas(std::max_align_t) unsigned char data[0];
    };
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif

    class StackGuard {
      private:
        lua_State *m_state;
        int m_expected;

      public:
        StackGuard(lua_State *state):
            m_state(state),
            m_expected(lua_gettop(state)) {
        }

        StackGuard(const StackGuard &) = delete;

        StackGuard(StackGuard &&) = delete;

        ~StackGuard(void) {
            int cur = lua_gettop(m_state);
            if (cur != m_expected) {
                argus_assert(lua_gettop(m_state) == m_expected);
            }
        }

        void increment(int count) {
            m_expected += count;
        }

        void increment(void) {
            increment(1);
        }

        void decrement(int count) {
            argus_assert(count <= m_expected);
            increment(-count);
        }

        void decrement(void) {
            decrement(1);
        }
    };

    [[nodiscard]] static ScriptCallbackResult _invoke_lua_function(
            lua_State *state,
            const std::vector<ObjectWrapper *> &params,
            const std::optional<std::string> &fn_name = std::nullopt
    );

    struct LuaCallback {
        std::weak_ptr<ManagedLuaState> m_state;
        int m_ref_key;

        LuaCallback(const std::shared_ptr<ManagedLuaState> &state, int index):
            m_state(state) {
            // duplicate the top stack value in order to leave the stack as we
            // found it
            lua_pushvalue(*state, index);
            m_ref_key = luaL_ref(*state, LUA_REGISTRYINDEX);
        }

        LuaCallback(const LuaCallback &rhs) = delete;

        ~LuaCallback() {
            auto state = m_state.lock();

            if (!state) {
                // Lua state was already destroyed, no need to clean up
                return;
            }

            luaL_unref(*state, LUA_REGISTRYINDEX, m_ref_key);
        }

        [[nodiscard]] ScriptCallbackResult call(
                const std::vector<ObjectWrapper *> &params) const {
            auto state = m_state.lock();

            if (!state) {
                crash("Attempt to invoke Lua callback after Lua state was destroyed");
            }

            StackGuard stack_guard(*state);

            lua_rawgeti(*state, LUA_REGISTRYINDEX, m_ref_key);

            return _invoke_lua_function(*state, params);
        }
    };

    static int _set_lua_error(lua_State *state, const std::string &msg) {
        return luaL_error(state, msg.c_str());
    }

    static std::string _string_or(const std::string& str, const std::string& def) {
        return !str.empty() ? str : def;
    }

    static std::string _get_metatable_name(lua_State *state, int index) {
        // get metatable of userdata
        lua_getmetatable(state, index);

        // get metatable name
        lua_pushstring(state, k_lua_name);
        lua_gettable(state, -2);
        const char *type_name = lua_tostring(state, -1);
        if (type_name == nullptr) {
            return "";
        }

        lua_pop(state, 2); // remove field name and metatable from stack

        return std::string(type_name);
    }

    static int _wrap_instance_ref(lua_State *state, const std::string &qual_fn_name,
            int param_index, const BoundTypeDef &type_def, bool require_mut, ObjectWrapper *dest) {
        if (!lua_isuserdata(state, param_index)) {
            return _set_lua_error(state, "Incorrect type provided for parameter "
                    + std::to_string(param_index) + " of function " + qual_fn_name
                    + " (expected " + type_def.name
                    + ", actual " + luaL_typename(state, param_index) + ")");
        }

        auto type_name = _get_metatable_name(state, param_index);
        if (!(type_name == type_def.name
                || (!require_mut && type_name == k_const_prefix + type_def.name))) {
            return _set_lua_error(state, "Incorrect userdata provided for parameter "
                    + std::to_string(param_index) + " of function " + qual_fn_name
                    + " (expected " + type_def.name
                    + ", actual " + _string_or(type_name, k_empty_repl) + ")");
        }

        auto *udata = reinterpret_cast<UserData *>(lua_touserdata(state, param_index));
        void *ptr;
        if (udata->is_handle) {
            ptr = deref_sv_handle(*reinterpret_cast<ScriptBindableHandle *>(udata->data), type_def.type_id);

            if (ptr == nullptr) {
                return _set_lua_error(state, "Invalid handle passed as parameter " + std::to_string(param_index)
                        + " of function " + qual_fn_name);
            }
        } else {
            ptr = static_cast<void *>(udata->data);
        }

        bool is_const = type_name.rfind(k_const_prefix, 0) == 0;

        ObjectType obj_type {
                IntegralType::Pointer,
                sizeof(void *),
                is_const,
                type_def.type_id,
                type_def.name
        };
        auto wrapper_res = create_object_wrapper(obj_type, &ptr);
        if (wrapper_res.is_err()) {
            return luaL_error(state, "Invalid arguments provided for function %s: %s",
                    qual_fn_name.c_str(), wrapper_res.unwrap_err().reason.c_str());
        }
        *dest = std::move(wrapper_res.unwrap());
        return 0;
    }

    template<typename T, typename U = T>
    static Result<ObjectWrapper, std::string> _wrap_prim_vector_param(lua_State *state, const ObjectType &param_def,
            const std::function<int(lua_State *, int)> &check_fn, const std::function<U(lua_State *, int)> &read_fn,
            const char *expected_type_name, int param_index, const std::string &qual_fn_name) {
        StackGuard stack_guard(state);

        // get number of indexed elements
        auto len = lua_rawlen(state, -1);
        affirm_precond(len <= INT_MAX, "Too many table indices");

        std::vector<T> vec;
        vec.reserve(len);

        for (size_t i = 0; i < len; i++) {
            auto index = int(i + 1);

            lua_rawgeti(state, -1, index);

            if (!check_fn(state, -1)) {
                lua_pop(state, 1);
                return err<ObjectWrapper, std::string>("Incorrect element type in vector parameter "
                        + std::to_string(param_index) + " of function " + qual_fn_name
                        + " (expected " + expected_type_name + ", actual " + luaL_typename(state, -1) + ")");
            }

            vec[i] = T(read_fn(state, -1));

            lua_pop(state, 1);
        }

        auto wrapper_res = create_vector_object_wrapper_from_stack(param_def, vec);

        if (wrapper_res.is_err()) {
            return err<ObjectWrapper, std::string>("Expected array type for parameter "
            + std::to_string(param_index) + " of function " + qual_fn_name);
        }

        return ok<ObjectWrapper, std::string>(wrapper_res.unwrap());
    }

    static Result<ObjectWrapper, std::string> _read_vector_from_table(lua_State *state,
            const std::string &qual_fn_name, int param_index, const ObjectType &param_def) {
        const auto &element_type = *param_def.primary_type.value();

        // for simplicity's sake we require contiguous indices

        switch (element_type.type) {
            case IntegralType::Integer:
            case IntegralType::UInteger:
            case IntegralType::Enum: {
                auto check_fn = [](auto *state, auto index) {
                    if (lua_isinteger(state, index)) {
                        return true;
                    } else if (lua_isnumber(state, index)) {
                        const double threshold = 1e-10;
                        auto num = lua_tonumber(state, index);
                        return fabs(num - round(num)) < threshold;
                    } else {
                        return false;
                    }
                };
                auto read_fn = [](auto *state, auto index) {
                    return lua_tointeger(state, index);
                };
                switch (element_type.size) {
                    case 1:
                        return _wrap_prim_vector_param<int8_t, lua_Integer>(state, param_def, check_fn, read_fn,
                                "integer", param_index, qual_fn_name);
                    case 2:
                        return _wrap_prim_vector_param<int16_t, lua_Integer>(state, param_def, check_fn, read_fn,
                                "integer", param_index, qual_fn_name);
                    case 4:
                        return _wrap_prim_vector_param<int32_t, lua_Integer>(state, param_def, check_fn, read_fn,
                                "integer", param_index, qual_fn_name);
                    case 8:
                        return _wrap_prim_vector_param<int64_t, lua_Integer>(state, param_def, check_fn, read_fn,
                                "integer", param_index, qual_fn_name);
                    default:
                        crash("Unknown integer width %s", element_type.size);
                }
            }
            case IntegralType::Float: {
                auto read_fn = [](auto *state, auto index) { return lua_tonumber(state, index); };
                switch (element_type.size) {
                    case 4:
                        return _wrap_prim_vector_param<float, lua_Number>(state, param_def, lua_isnumber,
                                read_fn, "number", param_index, qual_fn_name);
                    case 8:
                        return _wrap_prim_vector_param<double, lua_Number>(state, param_def, lua_isnumber,
                                read_fn, "number", param_index, qual_fn_name);
                    default:
                        crash("Unknown floating-point width %s", element_type.size);
                }
            }
            case IntegralType::String:
                return _wrap_prim_vector_param<std::string, const char *>(state, param_def, lua_isstring,
                        [](auto *state, auto index) { return lua_tostring(state, index); },
                        "string", param_index, qual_fn_name);
            case IntegralType::Struct:
            case IntegralType::Pointer: {
                // get number of indexed elements
                auto len = lua_rawlen(state, -1);
                affirm_precond(len <= INT_MAX, "Too many table indices");

                if (len == 0) {
                    return ok<ObjectWrapper, std::string>(create_vector_object_wrapper(param_def, nullptr, 0)
                            .expect("Failed to create object wrapper while reading vector from Lua VM"));
                }

                auto bound_type = ScriptManager::instance().get_bound_type_by_type_id(element_type.type_id.value())
                        .expect("Encountered unbound element type when reading vector from Lua VM");

                auto wrapper = ObjectWrapper(param_def, sizeof(ArrayBlob) + len * bound_type.size);

                auto &blob = wrapper.emplace<ArrayBlob>(element_type.size, len, bound_type.dtor);

                for (size_t i = 0; i < len; i++) {
                    auto index = int(i + 1);

                    lua_rawgeti(state, -1, index);

                    if (!lua_isuserdata(state, -1)) {
                        _set_lua_error(state, "Incorrect element type in parameter "
                                + std::to_string(param_index) + ", index " + std::to_string(index)
                                + " of function " + qual_fn_name
                                + " (expected userdata, actual " + luaL_typename(state, -1) + ")");
                    }

                    auto type_name = _get_metatable_name(state, -1);

                    if (!(type_name == element_type.type_name.value()
                            || (element_type.is_const
                                    && type_name == k_const_prefix + param_def.type_name.value()))) {
                        return err<ObjectWrapper, std::string>("Incorrect userdata provided in parameter "
                                + std::to_string(param_index) + ", index " + std::to_string(index)
                                + " of function " + qual_fn_name
                                + " (expected " + (param_def.is_const ? k_const_prefix : "")
                                + param_def.type_name.value()
                                + ", actual " + _string_or(type_name, k_empty_repl) + ")");
                    }

                    auto *udata = reinterpret_cast<UserData *>(lua_touserdata(state, -1));
                    void *ptr;
                    if (udata->is_handle) {
                        // userdata is storing handle of pointer to struct data
                        ptr = deref_sv_handle(*reinterpret_cast<ScriptBindableHandle *>(udata->data),
                                element_type.type_id.value());

                        if (ptr == nullptr) {
                            return err<ObjectWrapper, std::string>("Invalid handle passed in parameter "
                                    + std::to_string(param_index) + ", index " + std::to_string(index)
                                    + " of function " + qual_fn_name);
                        }
                    } else {
                        if (element_type.type == IntegralType::Pointer) {
                            //TODO: should we support this?
                            return err<ObjectWrapper, std::string>(
                                    "Cannot pass value-typed struct as pointer in parameter "
                                            + std::to_string(param_index) + ", index " + std::to_string(index)
                                            + " of function " + qual_fn_name);
                        }

                        // userdata is directly storing struct data
                        ptr = static_cast<void *>(udata->data);
                    }

                    if (element_type.type == IntegralType::Pointer) {
                        blob.set<void *>(i, ptr);
                    } else {
                        argus_assert(element_type.type == IntegralType::Struct);

                        if (bound_type.copy_ctor != nullptr) {
                            bound_type.copy_ctor(blob[i], ptr);
                        } else {
                            memcpy(blob[i], ptr, bound_type.size);
                        }
                    }

                    // pop value
                    lua_pop(state, 1);
                }

                return ok<ObjectWrapper, std::string>(wrapper);
            }
            default:
                crash("Unhandled element type ordinal "
                        + std::to_string(std::underlying_type_t<IntegralType>(element_type.type)));
        }
    }

    static Result<ObjectWrapper, std::string> _wrap_param(const std::shared_ptr<ManagedLuaState> &managed_state,
            const std::string &qual_fn_name, int param_index, const ObjectType &param_def) {
        lua_State *state = *managed_state;

        std::optional<Result<ObjectWrapper, ReflectiveArgumentsError>> wrapper_res;

        switch (param_def.type) {
            case IntegralType::Integer:
            case IntegralType::UInteger:
            case IntegralType::Enum: {
                if (!lua_isinteger(state, param_index)) {
                    return err<ObjectWrapper, std::string>("Incorrect type provided for parameter "
                            + std::to_string(param_index) + " of function " + qual_fn_name
                            + " (expected integer " + (param_def.type == IntegralType::Enum ? "(enum) " : "")
                            + ", actual " + luaL_typename(state, param_index) + ")");
                }

                wrapper_res = create_int_object_wrapper(param_def, lua_tointeger(state, param_index));

                break;
            }
            case IntegralType::Float: {
                if (!lua_isnumber(state, param_index)) {
                    return err<ObjectWrapper, std::string>("Incorrect type provided for parameter "
                            + std::to_string(param_index) + " of function " + qual_fn_name
                            + " (expected number, actual " + luaL_typename(state, param_index) + ")");
                }

                wrapper_res = create_float_object_wrapper(param_def, lua_tonumber(state, param_index));

                break;
            }
            case IntegralType::Boolean: {
                if (!lua_isboolean(state, param_index)) {
                    return err<ObjectWrapper, std::string>("Incorrect type provided for parameter "
                            + std::to_string(param_index) + " of function " + qual_fn_name
                            + " (expected boolean, actual " + luaL_typename(state, param_index) + ")");
                }

                wrapper_res = create_bool_object_wrapper(param_def, lua_toboolean(state, param_index));

                break;
            }
            case IntegralType::String: {
                if (!lua_isstring(state, param_index)) {
                    return err<ObjectWrapper, std::string>("Incorrect type provided for parameter "
                            + std::to_string(param_index) + " of function " + qual_fn_name
                            + " (expected string, actual " + luaL_typename(state, param_index) + ")");
                }

                wrapper_res = create_string_object_wrapper(param_def, lua_tostring(state, param_index));

                break;
            }
            case IntegralType::Struct:
            case IntegralType::Pointer: {
                argus_assert(param_def.type_name.has_value());
                argus_assert(param_def.type_id.has_value());

                if (!lua_isuserdata(state, param_index)) {
                    return err<ObjectWrapper, std::string>("Incorrect type provided for parameter "
                            + std::to_string(param_index) + " of function " + qual_fn_name
                            + " (expected userdata, actual " + luaL_typename(state, param_index) + ")");
                }

                auto type_name = _get_metatable_name(state, param_index);

                if (!(type_name == param_def.type_name.value()
                        || (param_def.is_const && type_name == k_const_prefix + param_def.type_name.value()))) {
                    return err<ObjectWrapper, std::string>("Incorrect userdata provided for parameter "
                            + std::to_string(param_index) + " of function " + qual_fn_name
                            + " (expected " + (param_def.is_const ? k_const_prefix : "") +
                            param_def.type_name.value()
                            + ", actual " + _string_or(type_name, k_empty_repl) + ")");
                }

                auto *udata = reinterpret_cast<UserData *>(lua_touserdata(state, param_index));
                void *ptr;
                if (udata->is_handle) {
                    // userdata is storing handle of pointer to struct data
                    ptr = deref_sv_handle(*reinterpret_cast<ScriptBindableHandle *>(udata->data),
                            param_def.type_id.value());

                    if (ptr == nullptr) {
                        return err<ObjectWrapper, std::string>("Invalid handle passed as parameter "
                                + std::to_string(param_index) + " of function " + qual_fn_name);
                    }
                } else {
                    ptr = udata->data;
                }

                if (param_def.type == IntegralType::Struct) {
                    // pass direct pointer so that the struct data is copied
                    // into the ObjectWrapper
                    wrapper_res = create_object_wrapper(param_def, ptr);
                } else {
                    // pass indirect pointer so that the pointer itself is
                    // copied into the ObjectWrapper
                    wrapper_res = create_object_wrapper(param_def, &ptr);
                }

                break;
            }
            case IntegralType::Callback: {
                /*if (!lua_isfunction(state, param_index)) {
                    return _set_lua_error(state, "Incorrect type provided for parameter "
                                                 + std::to_string(param_index) + " of function " + qual_fn_name
                                                 + " (expected function, actual "
                                                 + luaL_typename(state, param_index) + ")");
                }*/

                auto handle = new LuaCallback(managed_state, param_index);

                BareProxiedScriptCallback fn = [](
                        size_t params_count,
                        ObjectWrapper **params,
                        const void *data,
                        ScriptCallbackResult *out_result
                ) {
                    std::vector<ObjectWrapper *> params_vec(params, params + params_count);
                    *out_result = ScriptCallbackResult(
                            reinterpret_cast<const LuaCallback *>(data)->call(params_vec)
                    );
                };

                wrapper_res = create_callback_object_wrapper(param_def, ProxiedScriptCallback { fn, handle });

                break;
            }
            case IntegralType::Type: {
                if (!lua_istable(state, param_index)) {
                    return err<ObjectWrapper, std::string>("Incorrect type provided for parameter "
                            + std::to_string(param_index) + " of function " + qual_fn_name
                            + " (expected table, actual " + luaL_typename(state, param_index));
                }

                lua_pushvalue(state, param_index);

                lua_getfield(state, param_index, k_lua_name);

                if (!lua_isstring(state, -1)) {
                    lua_pop(state, 2); // pop type name and table
                    return err<ObjectWrapper, std::string>("Parameter " + std::to_string(param_index)
                            + " does not represent type (missing field '" + k_lua_name + "')");
                }

                const char *type_name = lua_tostring(state, -1);

                lua_pop(state, 2); // pop type name and table

                auto bound_type_res = ScriptManager::instance().get_bound_type_by_name(type_name);
                if (bound_type_res.is_ok()) {
                    auto bound_type_index = bound_type_res.unwrap().type_id;
                    wrapper_res = create_object_wrapper(param_def, static_cast<const void *>(&bound_type_index));

                    break;
                } else {
                    return err<ObjectWrapper, std::string>("Unknown type '" + std::string(type_name) + " passed as parameter "
                            + std::to_string(param_index) + " of function " + qual_fn_name);
                }
            }
            case IntegralType::Vector:
            case IntegralType::VectorRef: {
                argus_assert(param_def.primary_type.has_value());

                if (lua_istable(state, param_index)) {
                    return _read_vector_from_table(state, qual_fn_name, param_index, param_def);
                } else if (lua_isuserdata(state, param_index)) {
                    auto type_name = _get_metatable_name(state, param_index);

                    if (type_name != k_mt_vector_ref) {
                        return err<ObjectWrapper, std::string>("Incorrect type provided for parameter "
                                + std::to_string(param_index) + " of function " + qual_fn_name
                                + " (expected VectorWrapper, actual " + _string_or(type_name, k_empty_repl) + ")");
                    }

                    ObjectType real_type = param_def;
                    real_type.type = IntegralType::VectorRef;
                    VectorWrapper *vec = reinterpret_cast<VectorWrapper *>(lua_touserdata(state, param_index));
                    wrapper_res = create_vector_ref_object_wrapper(real_type, *vec);

                    break;
                } else {
                    return err<ObjectWrapper, std::string>("Incorrect type provided for parameter "
                            + std::to_string(param_index) + " of function " + qual_fn_name
                            + " (expected table or userdata, actual " + luaL_typename(state, param_index) + ")");
                }
            }
            default:
                crash("Unknown integral type ordinal %d\n", param_def.type);
        }

        if (wrapper_res->is_err()) {
            return err<ObjectWrapper, std::string>("Invalid value passed to for parameter "
                    + std::to_string(param_index) + " of function " + qual_fn_name
                    + "(" + wrapper_res->unwrap_err().reason + ")");
        }

        return ok<ObjectWrapper, std::string>(wrapper_res->unwrap());
    }

    static int64_t _unwrap_int_wrapper(const ObjectWrapper &wrapper) {
        argus_assert(wrapper.type.type == IntegralType::Integer
                || wrapper.type.type == IntegralType::UInteger
                || wrapper.type.type == IntegralType::Enum);

        switch (wrapper.type.size) {
            case 1:
                return int64_t(*reinterpret_cast<const int8_t *>(wrapper.value));
            case 2:
                return int64_t(*reinterpret_cast<const int16_t *>(wrapper.value));
            case 4:
                return int64_t(*reinterpret_cast<const int32_t *>(wrapper.value));
            case 8:
                return *reinterpret_cast<const int64_t *>(wrapper.value);
            default:
                crash("Bad integer width " + std::to_string(wrapper.type.size)
                        + " (must be 1, 2, 4, or 8)");
        }
    }

    static double _unwrap_float_wrapper(const ObjectWrapper &wrapper) {
        argus_assert(wrapper.type.type == IntegralType::Float);

        switch (wrapper.type.size) {
            case 4:
                return double(*reinterpret_cast<const float *>(wrapper.value));
            case 8:
                return *reinterpret_cast<const double *>(wrapper.value);
            default:
                crash("Bad floating-point width " + std::to_string(wrapper.type.size)
                        + " (must be 4, or 8)");
        }
    }

    static bool _unwrap_boolean_wrapper(const ObjectWrapper &wrapper) {
        argus_assert(wrapper.type.type == IntegralType::Boolean);

        return *reinterpret_cast<const bool *>(wrapper.value);
    }

    static void _set_metatable(lua_State *state, const ObjectType &type) {
        auto mt = luaL_getmetatable(state,
                ((type.is_const ? k_const_prefix : "") + type.type_name.value()).c_str());
        UNUSED(mt);
        argus_assert(mt != 0); // binding should have failed if type wasn't bound

        lua_setmetatable(state, -2);
    }

    static void _push_value(lua_State *state, const ObjectWrapper &wrapper);

    static int _lua_vector_index_handler(lua_State *state) {
        VectorWrapper *vec = reinterpret_cast<VectorWrapper *>(lua_touserdata(state, -2));
        auto index = lua_tointeger(state, -1);

        auto vec_size = vec->get_size();
        if (index <= 0 || size_t(index) > vec_size) {
            return _set_lua_error(state, "Index out of range for vector of size " + std::to_string(vec_size));
        }

        const void *el_ptr = const_cast<const VectorWrapper *>(vec)->at(size_t(index) - 1);
        if (vec->element_type().type == IntegralType::Pointer) {
            el_ptr = *reinterpret_cast<const void *const *>(el_ptr);
        }

        if (vec->element_type().type == IntegralType::Struct) {
            // hack to return a reference to the vector element instead of a copy
            ObjectType modified_type = vec->element_type();
            modified_type.type = IntegralType::Pointer;

            auto wrapper_res = create_object_wrapper(modified_type, el_ptr);
            _push_value(state, wrapper_res.expect("Failed to create object wrapper while accessing vector element"));
        } else {
            auto wrapper_res = create_object_wrapper(vec->element_type(), el_ptr);
            _push_value(state, wrapper_res.expect("Failed to create object wrapper while accessing vector element"));
        }

        return 1;
    }

    static int _lua_vector_ro_newindex_handler(lua_State *state) {
        return _set_lua_error(state, "Cannot modify read-only vector returned from a bound function");
    }

    static int _lua_vector_rw_newindex_handler(lua_State *state) {
        VectorWrapper *vec = reinterpret_cast<VectorWrapper *>(lua_touserdata(state, -3));
        auto index = lua_tointeger(state, -2);

        auto vec_size = vec->get_size();
        if (index <= 0 || size_t(index) > vec_size) {
            return _set_lua_error(state, "Index out of range for vector of size " + std::to_string(vec_size));
        }

        auto wrapper_res = _wrap_param(to_managed_state(state), "__newindex", -1, vec->element_type());
        if (wrapper_res.is_err()) {
            return _set_lua_error(state, wrapper_res.unwrap_err());
        }

        vec->set(size_t(index) - 1, wrapper_res.unwrap().get_ptr0());

        return 1;
    }

    static int _lua_result_is_ok_handler(lua_State *state) {
        StackGuard stack_guard(state);

        void *udata_ptr = lua_touserdata(state, -1);

        if (udata_ptr == nullptr) {
            return luaL_error(state, "Result methods may not be statically invoked (use the colon operator instead)");
        }

        const ResultWrapper *res = reinterpret_cast<const ResultWrapper *>(udata_ptr);

        lua_pushboolean(state, res->is_ok());
        stack_guard.increment();

        return 1;
    }

    static int _lua_result_is_err_handler(lua_State *state) {
        StackGuard stack_guard(state);

        void *udata_ptr = lua_touserdata(state, -1);

        if (udata_ptr == nullptr) {
            return luaL_error(state, "Result methods may not be statically invoked (use the colon operator instead)");
        }

        const ResultWrapper *res = reinterpret_cast<const ResultWrapper *>(udata_ptr);

        lua_pushboolean(state, !res->is_ok());
        stack_guard.increment();

        return 1;
    }

    static int _lua_result_unwrap_handler(lua_State *state) {
        StackGuard stack_guard(state);

        void *udata_ptr = lua_touserdata(state, -1);

        if (udata_ptr == nullptr) {
            return luaL_error(state, "Result methods may not be statically invoked (use the colon operator instead)");
        }

        const ResultWrapper *res = reinterpret_cast<const ResultWrapper *>(udata_ptr);

        if (!res->is_ok()) {
            int err_inc = luaL_error(state, "Cannot unwrap value from error-typed result");
            stack_guard.increment(err_inc);
            return err_inc;
        }

        _push_value(state,
                res->to_object_wrapper().expect("Failed to create object wrapper while unwrapping result value"));
        stack_guard.increment();

        return 1;
    }

    static int _lua_result_unwrap_err_handler(lua_State *state) {
        StackGuard stack_guard(state);

        void *udata_ptr = lua_touserdata(state, -1);

        if (udata_ptr == nullptr) {
            return luaL_error(state, "Result methods may not be statically invoked (use the colon operator instead)");
        }

        const ResultWrapper *res = reinterpret_cast<const ResultWrapper *>(udata_ptr);

        if (res->is_ok()) {
            int err_inc = luaL_error(state, "Cannot unwrap error from value-typed result");
            stack_guard.increment(err_inc);
            return err_inc;
        }

        _push_value(state,
                res->to_object_wrapper().expect("Failed to create object wrapper while unwrapping result error"));
        stack_guard.increment();

        return 1;
    }

    static int _lua_result_expect_handler(lua_State *state) {
        StackGuard stack_guard(state);

        auto arg_count = lua_gettop(state) - 1;

        void *udata_ptr = lua_touserdata(state, -1 - arg_count);

        if (udata_ptr == nullptr) {
            return luaL_error(state, "Result methods may not be statically invoked (use the colon operator instead)");
        }

        const ResultWrapper *res = reinterpret_cast<const ResultWrapper *>(udata_ptr);

        if (!res->is_ok()) {
            int err_inc = luaL_error(state, arg_count > 0 ? lua_tostring(state, -1) : "Expectation failed");
            stack_guard.increment(err_inc);
            return err_inc;
        }

        _push_value(state,
                res->to_object_wrapper().expect("Failed to create object wrapper while unwrapping result error"));
        stack_guard.increment();

        return 1;
    }

    static int _lua_result_expect_err_handler(lua_State *state) {
        StackGuard stack_guard(state);

        auto arg_count = lua_gettop(state) - 1;

        void *udata_ptr = lua_touserdata(state, -1 - arg_count);

        if (udata_ptr == nullptr) {
            return luaL_error(state, "Result methods may not be statically invoked (use the colon operator instead)");
        }

        const ResultWrapper *res = reinterpret_cast<const ResultWrapper *>(udata_ptr);

        if (res->is_ok()) {
            int err_inc = luaL_error(state, arg_count > 0 ? lua_tostring(state, -1) : "Expectation failed");
            stack_guard.increment(err_inc);
            return err_inc;
        }

        _push_value(state,
                res->to_object_wrapper().expect("Failed to create object wrapper while unwrapping result error"));
        stack_guard.increment();

        return 1;
    }

    static int _lua_result_index_handler(lua_State *state) {
        StackGuard guard(state);

        const char *method_name = lua_tostring(state, -1);

        if (strcmp(method_name, k_result_is_ok_fn) == 0) {
            lua_pushcfunction(state, _lua_result_is_ok_handler);
            guard.increment();
            return 1;
        } else if (strcmp(method_name, k_result_is_err_fn) == 0) {
            lua_pushcfunction(state, _lua_result_is_err_handler);
            guard.increment();
            return 1;
        } else if (strcmp(method_name, k_result_unwrap_fn) == 0) {
            lua_pushcfunction(state, _lua_result_unwrap_handler);
            guard.increment();
            return 1;
        } else if (strcmp(method_name, k_result_unwrap_err_fn) == 0) {
            lua_pushcfunction(state, _lua_result_unwrap_err_handler);
            guard.increment();
            return 1;
        } else if (strcmp(method_name, k_result_expect_fn) == 0) {
            lua_pushcfunction(state, _lua_result_expect_handler);
            guard.increment();
            return 1;
        } else if (strcmp(method_name, k_result_expect_err_fn) == 0) {
            lua_pushcfunction(state, _lua_result_expect_err_handler);
            guard.increment();
            return 1;
        } else {
            auto err_count = luaL_error(state,
                    "Index '%s' does not exist in result type (make sure to unwrap the result before using it)",
                    method_name);
            guard.increment(err_count);
            return err_count;
        }
    }

    static void _push_vector_vals(lua_State *state, const ObjectType &element_type, const ArrayBlob &vec) {
        argus_assert(vec.size() < INT_MAX);
        for (size_t i = 0; i < vec.size(); i++) {
            // push index to stack
            lua_pushinteger(state, int(i + 1));
            switch (element_type.type) {
                case IntegralType::Integer:
                case IntegralType::UInteger:
                case IntegralType::Enum:
                    switch (vec.element_size()) {
                        case 1:
                            lua_pushinteger(state, vec.at<int8_t>(i));
                            break;
                        case 2:
                            lua_pushinteger(state, vec.at<int16_t>(i));
                            break;
                        case 4:
                            lua_pushinteger(state, vec.at<int32_t>(i));
                            break;
                        case 8:
                            lua_pushinteger(state, vec.at<int64_t>(i));
                            break;
                        default:
                            crash("Unhandled int width " + std::to_string(vec.element_size()) + " in vector");
                    }
                    break;
                case IntegralType::Float:
                    lua_pushnumber(state, vec.element_size() == 8 ? vec.at<double>(i) : vec.at<float>(i));
                    break;
                case IntegralType::Boolean:
                    lua_pushboolean(state, vec.at<bool>(i));
                    break;
                case IntegralType::String:
                    lua_pushstring(state, vec.at<std::string>(i).c_str());
                    break;
                case IntegralType::Struct: {
                    argus_assert(element_type.type_name.has_value());

                    auto *udata = reinterpret_cast<UserData *>(lua_newuserdata(state,
                            sizeof(UserData) + element_type.size));
                    udata->is_handle = false;

                    auto bound_type = ScriptManager::instance().get_bound_type_by_type_id(element_type.type_id.value())
                            .expect("Tried to wrap parameter of unbound struct type");
                    if (bound_type.copy_ctor != nullptr) {
                        bound_type.copy_ctor(udata->data, vec[i]);
                    } else {
                        memcpy(udata->data, vec[i], vec.element_size());
                    }
                    _set_metatable(state, element_type);

                    break;
                }
                case IntegralType::Pointer: {
                    auto ptr = vec.at<void *>(i);
                    if (ptr != nullptr) {
                        auto handle = get_or_create_sv_handle(element_type.type_id.value(), ptr);
                        auto *udata = reinterpret_cast<UserData *>(lua_newuserdata(state,
                                sizeof(UserData) + sizeof(ScriptBindableHandle)));
                        udata->is_handle = true;
                        memcpy(udata->data, &handle, sizeof(ScriptBindableHandle));
                        _set_metatable(state, element_type);
                    } else {
                        lua_pushnil(state);
                    }

                    break;
                }
                default:
                    // remove key from stack
                    lua_pop(state, 1);
                    crash("Unhandled element type ordinal "
                            + std::to_string(std::underlying_type_t<IntegralType>(element_type.type)));
                    break;
            }

            // add key-value pair to table
            lua_settable(state, -3);
        }
    }

    static void _push_result(lua_State *state, const ResultWrapper &result) {
        // create userdata to return
        ResultWrapper *udata = reinterpret_cast<ResultWrapper *>(
                lua_newuserdata(state, sizeof(ResultWrapper) + result.get_size()));
        new(udata) ResultWrapper(result);

        // create metatable
        luaL_newmetatable(state, k_mt_result);

        // indexing is handled by a delgating function instead of a dispatch
        // table so that we can display a nice error message when a script tries
        // to use a result directly

        // push index handler function
        lua_pushcfunction(state, _lua_result_index_handler);
        // bind handler to metatable
        lua_setfield(state, -2, k_lua_index);

        // bind metatable to userdata
        lua_setmetatable(state, -2);

        // table is now on top of stack
    }

    static void _push_value(lua_State *state, const ObjectWrapper &wrapper) {
        argus_assert(wrapper.type.type != IntegralType::Void);

        switch (wrapper.type.type) {
            case IntegralType::Integer:
            case IntegralType::UInteger:
            case IntegralType::Enum:
                lua_pushinteger(state, _unwrap_int_wrapper(wrapper));
                break;
            case IntegralType::Float:
                lua_pushnumber(state, _unwrap_float_wrapper(wrapper));
                break;
            case IntegralType::Boolean:
                lua_pushboolean(state, _unwrap_boolean_wrapper(wrapper));
                break;
            case IntegralType::String:
                lua_pushstring(state, reinterpret_cast<const char *>(wrapper.get_ptr0()));
                break;
            case IntegralType::Struct: {
                argus_assert(wrapper.type.type_name.has_value());

                auto *udata = reinterpret_cast<UserData *>(lua_newuserdata(state,
                        sizeof(UserData) + wrapper.type.size));
                udata->is_handle = false;
                wrapper.copy_value_into(udata->data, wrapper.type.size);
                _set_metatable(state, wrapper.type);

                break;
            }
            case IntegralType::Pointer: {
                argus_assert(wrapper.type.type_id.has_value());
                argus_assert(wrapper.type.type_name.has_value());

                void *ptr = *reinterpret_cast<void *const *>(wrapper.get_ptr0());

                if (ptr != nullptr) {
                    auto handle = get_or_create_sv_handle(wrapper.type.type_id.value(), ptr);
                    auto *udata = reinterpret_cast<UserData *>(lua_newuserdata(state,
                            sizeof(UserData) + sizeof(ScriptBindableHandle)));
                    udata->is_handle = true;
                    memcpy(udata->data, &handle, sizeof(ScriptBindableHandle));
                    _set_metatable(state, wrapper.type);
                } else {
                    lua_pushnil(state);
                }

                break;
            }
            case IntegralType::Vector: {
                auto &vec = wrapper.get_value<ArrayBlob>();
                affirm_precond(vec.size() <= INT_MAX, "Vector is too big");

                // create table to return
                lua_createtable(state, int(vec.size()), 0);

                argus_assert(wrapper.type.primary_type.has_value());
                _push_vector_vals(state, *wrapper.type.primary_type.value(), vec);

                // create metatable
                luaL_newmetatable(state, k_mt_vector);
                // set __newindex override
                lua_pushcfunction(state, _lua_vector_ro_newindex_handler);
                lua_setfield(state, -2, k_lua_newindex);
                // set metatable on return table
                lua_setmetatable(state, -2);

                // table is now on top of stack
                break;
            }
            case IntegralType::VectorRef: {
                auto &vec = *reinterpret_cast<const VectorWrapper *>(wrapper.get_ptr0());

                // create userdata to return
                VectorWrapper *udata = reinterpret_cast<VectorWrapper *>(lua_newuserdata(state, sizeof(VectorWrapper)));
                new(udata) VectorWrapper(vec);

                // create metatable
                luaL_newmetatable(state, k_mt_vector_ref);
                // set __index override
                lua_pushcfunction(state, _lua_vector_index_handler);
                lua_setfield(state, -2, k_lua_index);
                // set __newindex override
                if (vec.is_const()) {
                    lua_pushcfunction(state, _lua_vector_ro_newindex_handler);
                } else {
                    lua_pushcfunction(state, _lua_vector_rw_newindex_handler);
                }
                lua_setfield(state, -2, k_lua_newindex);
                // set metatable on return table
                lua_setmetatable(state, -2);

                // table is now on top of stack
                break;
            }
            case IntegralType::Result: {
                const auto &result = wrapper.get_value<ResultWrapper>();

                _push_result(state, result);

                break;
            }
            default:
                argus_assert(false);
        }
    }

    [[nodiscard]] static ScriptCallbackResult _invoke_lua_function(lua_State *state,
            const std::vector<ObjectWrapper *> &params, const std::optional<std::string> &fn_name) {
        for (const auto *param : params) {
            _push_value(state, *param);
        }

        if (lua_pcall(state, int(params.size()), 0, 0) != LUA_OK) {
            auto err_msg = lua_tostring(state, -1);
            lua_pop(state, 1); // pop error message
            return ScriptCallbackResult {
                false,
                std::nullopt,
                ScriptInvocationError { fn_name.value_or("callback"), err_msg }
            };
        }

        ObjectType type {
                IntegralType::Void,
                0
        };
        return ScriptCallbackResult {
                true,
                ObjectWrapper { type, 0 },
                std::nullopt
        };
    }

    static int _lua_trampoline(lua_State *state) {
        StackGuard stack_guard(state);

        auto fn_type = static_cast<FunctionType>(lua_tointeger(state, lua_upvalueindex(1)));
        if (fn_type != FunctionType::Global && fn_type != FunctionType::MemberInstance
                && fn_type != FunctionType::MemberStatic && fn_type != FunctionType::Extension) {
            crash("Popped unknown function type value from Lua stack");
        }

        std::string type_name;
        auto fn_name_index = 2;
        if (fn_type != FunctionType::Global) {
            type_name = lua_tostring(state, lua_upvalueindex(2));
            fn_name_index = 3;
        }

        std::string fn_name(lua_tostring(state, lua_upvalueindex(fn_name_index)));

        auto qual_fn_name = get_qualified_function_name(fn_type, type_name, fn_name);

        auto &mgr = ScriptManager::instance();

        std::optional<Result<const BoundFunctionDef &, SymbolNotBoundError>> fn_res;
        switch (fn_type) {
            case FunctionType::Global:
                fn_res = mgr.get_native_global_function(fn_name);
                break;
            case FunctionType::MemberInstance:
                fn_res = mgr.get_native_member_instance_function(type_name, fn_name);
                break;
            case FunctionType::Extension:
                fn_res = mgr.get_native_extension_function(type_name, fn_name);
                break;
            case FunctionType::MemberStatic:
                fn_res = mgr.get_native_member_static_function(type_name, fn_name);
                break;
            default:
                crash("Unknown function type ordinal %d", fn_type);
        }

        if (fn_res->is_err()) {
            const char *symbol_type_disp;
            switch (fn_res->unwrap_err().symbol_type) {
                case SymbolType::Type:
                    symbol_type_disp = "Type";
                    break;
                case SymbolType::Field:
                    symbol_type_disp = "Field";
                    break;
                case SymbolType::Function:
                    symbol_type_disp = "Function";
                    break;
                default:
                    symbol_type_disp = "Symbol";
                    break;
            }

            return luaL_error(state, "%s with name %s is not bound",
                    symbol_type_disp, fn_res->unwrap_err().symbol_name.c_str());
        }

        const auto &fn = fn_res->unwrap();

        // parameter count not including instance
        auto arg_count = lua_gettop(state);
        auto expected_arg_count = fn.params.size()
                + (fn.type == FunctionType::MemberInstance ? 1 : 0);
        if (size_t(arg_count) != expected_arg_count) {
            auto err_msg = "Wrong parameter count provided for function " + qual_fn_name
                    + " (expected " + std::to_string(expected_arg_count)
                    + ", actual " + std::to_string(arg_count) + ")";

            if ((fn_type == FunctionType::MemberInstance || fn_type == FunctionType::Extension)
                    && expected_arg_count == uint32_t(arg_count + 1)) {
                err_msg += " (did you forget to use the colon operator?)";
            }

            return _set_lua_error(state, err_msg);
        }

        // calls to instance member functions push the instance as the first "parameter"
        auto first_param_index = fn_type == FunctionType::MemberInstance ? 1 : 0;

        std::vector<ObjectWrapper> args;

        if (fn_type == FunctionType::MemberInstance) {
            // type should definitely be bound since the trampoline function
            // is accessed via the bound type's metatable
            auto type_def = ScriptManager::instance().get_bound_type_by_name(type_name)
                    .expect("Failed to find bound type while handling bound instance function");

            //TODO: add safeguard to prevent invocation of functions on non-references
            ObjectWrapper wrapper {};
            // 5th param is whether the instance must be mutable, which is
            // the case iff the function is non-const
            auto wrap_res = _wrap_instance_ref(state, qual_fn_name, 1, type_def, !fn.is_const, &wrapper);
            if (wrap_res == 0) {
                args.emplace_back(std::move(wrapper));
            } else {
                // some error occurred
                // _wrap_instance_ref already sent error to Lua state
                return wrap_res;
            }
        }

        for (int i = 0; i < (arg_count - first_param_index); i++) {
            // Lua is 1-indexed, also add offset to skip instance parameter if present
            auto param_index = i + 1 + first_param_index;
            auto param_def = fn.params.at(size_t(i));

            auto wrapper_res = _wrap_param(to_managed_state(state), qual_fn_name, param_index, param_def);
            if (wrapper_res.is_err()) {
                return _set_lua_error(state, wrapper_res.unwrap_err());
            }

            args.emplace_back(std::move(wrapper_res.unwrap()));
        }

        auto retval_res = fn.handle(args);

        if (retval_res.is_err()) {
            return _set_lua_error(state, "Bad arguments provided to function " + qual_fn_name
                    + " (" + retval_res.unwrap_err().reason + ")");
        }

        auto retval = std::move(retval_res.unwrap());

        if (retval.type.type != IntegralType::Void) {
            _push_value(state, retval);
            stack_guard.increment();

            return 1;
        } else {
            return 0;
        }
    }

    static int _lookup_fn_in_dispatch_table(lua_State *state, int mt_index, int key_index) {
        // get value from type's dispatch table instead
        // get type's metatable
        lua_getmetatable(state, mt_index);
        // get dispatch table
        lua_getmetatable(state, -1);
        lua_remove(state, -2);
        // push key onto stack
        lua_pushvalue(state, key_index);
        // get value of key from metatable
        lua_rawget(state, -2);
        lua_remove(state, -2);

        return 1;
    }

    static bool _get_native_field_val(lua_State *state, const std::string &type_name, const std::string &field_name) {
        StackGuard stack_guard(state);

        auto real_type_name = type_name.rfind(k_const_prefix, 0) == 0
                ? type_name.substr(strlen(k_const_prefix))
                : type_name;

        auto field_res = ScriptManager::instance().get_native_member_field(real_type_name, field_name);
        if (field_res.is_err()) {
            return false;
        }

        const auto &field = field_res.unwrap();

        auto qual_field_name = get_qualified_field_name(real_type_name, field_name);

        // type should definitely be bound since the field is accessed through
        // its associated metatable
        auto type_def = ScriptManager::instance().get_bound_type_by_name(real_type_name)
                .expect("Failed to find type while accessing field");

        ObjectWrapper inst_wrapper {};
        auto wrap_res = _wrap_instance_ref(state, qual_field_name, 1, type_def,
                false, &inst_wrapper);
        if (wrap_res != 0) {
            // some error occurred
            // _wrap_instance_ref already sent error to lua state
            return wrap_res;
        }

        auto val = field.get_value(inst_wrapper);

        _push_value(state, val);
        stack_guard.increment();

        return true;
    }

    static int _lua_type_index_handler(lua_State *state) {
        StackGuard stack_guard(state);

        std::string type_name = _get_metatable_name(state, 1);
        std::string key = lua_tostring(state, -1);

        argus_assert(!type_name.empty());

        if (_get_native_field_val(state, type_name, key)) {
            stack_guard.increment();
            return 1;
        } else {
            auto retval = _lookup_fn_in_dispatch_table(state, 1, 2);
            stack_guard.increment(retval);
            return retval;
        }
    }

    // assumes the value is at the top of the stack
    static int _set_native_field(lua_State *state, const std::string &type_name, const std::string &field_name) {
        StackGuard stack_guard(state);

        // only necessary for the error message when the object is const since that's the only time it has the prefix
        auto real_type_name = type_name.rfind(k_const_prefix, 0) == 0
                ? type_name.substr(strlen(k_const_prefix))
                : type_name;

        auto qual_field_name = get_qualified_field_name(real_type_name, field_name);

        // can't assign fields of a const object
        if (type_name.rfind(k_const_prefix, 0) == 0) {
            return _set_lua_error(state, "Field " + qual_field_name + " in a const object cannot be assigned");
        }

        auto field_res = ScriptManager::instance().get_native_member_field(type_name, field_name);
        if (field_res.is_err()) {
            return _set_lua_error(state, "Field " + qual_field_name + " is not bound");
        }

        const auto &field = field_res.unwrap();

        // can't assign a const field
        if (field.m_type.is_const) {
            return _set_lua_error(state, "Field " + qual_field_name + " is const and cannot be assigned");
        }

        // type should definitely be bound since the field is accessed through
        // its associated metatable
        auto type_def = ScriptManager::instance().get_bound_type_by_name(type_name).expect("Failed to find bound type while setting field");

        ObjectWrapper inst_wrapper {};
        auto wrap_res = _wrap_instance_ref(state, qual_field_name, 1, type_def, true, &inst_wrapper);
        if (wrap_res != 0) {
            // some error occurred
            // _wrap_instance_ref already sent error to lua state, so just clean up here
            return wrap_res;
        }

        auto val_wrapper_res = _wrap_param(to_managed_state(state), qual_field_name, -1, field.m_type);
        if (val_wrapper_res.is_err()) {
            return _set_lua_error(state, val_wrapper_res.unwrap_err());
        }

        argus_assert(field.m_assign_proxy.has_value());
        field.m_assign_proxy.value()(inst_wrapper, val_wrapper_res.unwrap());

        return 0;
    }

    static int _lua_type_newindex_handler(lua_State *state) {
        StackGuard stack_guard(state);

        std::string type_name = _get_metatable_name(state, 1);
        std::string key = lua_tostring(state, -2);

        argus_assert(!type_name.empty());

        auto res = _set_native_field(state, type_name, key);

        return res;
    }

    static int _clone_object(lua_State *state) {
        StackGuard stack_guard(state);

        std::string type_name = _get_metatable_name(state, 1);
        if (type_name.rfind(k_const_prefix, 0) == 0) {
            type_name = type_name.substr(strlen(k_const_prefix));
        }

        auto param_count = lua_gettop(state);
        if (param_count != 1) {
            std::string msg = "Wrong parameter count for function clone";
            if (lua_gettop(state)) {
                msg += " (did you forget to use the colon operator?)";
            }
            return _set_lua_error(state, msg);
        }

        if (!lua_isuserdata(state, -1)) {
            return _set_lua_error(state, "clone() called on non-userdata object");
        }

        // type should definitely be bound since we're getting it directly from
        // its associated metatable
        auto type_def_res = ScriptManager::instance().get_bound_type_by_name(type_name);
        const auto &type_def = type_def_res.expect("Failed to find type while cloning object");
        if (type_def.copy_ctor == nullptr) {
            return _set_lua_error(state, type_name + " is not cloneable");
        }

        UserData *udata = reinterpret_cast<UserData *>(lua_touserdata(state, -1));

        void *src;
        if (udata->is_handle) {
            auto handle = reinterpret_cast<ScriptBindableHandle *>(udata->data);
            src = deref_sv_handle(*handle, type_def.type_id);
        } else {
            src = udata->data;
        }

        UserData *dest = reinterpret_cast<UserData *>(lua_newuserdata(state, sizeof(UserData) + type_def.size));
        dest->is_handle = false;
        stack_guard.increment();
        auto mt = luaL_getmetatable(state, type_def.name.c_str());
        UNUSED(mt);
        argus_assert(mt != 0); // binding should have failed if type wasn't bound
        lua_setmetatable(state, -2);

        type_def.copy_ctor(dest->data, src);

        return 1;
    }

    static void _bind_fn(lua_State *state, const BoundFunctionDef &fn, const std::string &type_name) {
        // push function type
        lua_pushinteger(state, uint32_t(fn.type));
        // push type name (only if member function)
        if (fn.type != FunctionType::Global) {
            lua_pushstring(state, type_name.c_str());
        }
        // push function name
        lua_pushstring(state, fn.name.c_str());

        auto upvalue_count = fn.type == FunctionType::Global ? 2 : 3;

        lua_pushcclosure(state, _lua_trampoline, upvalue_count);

        lua_setfield(state, -2, fn.name.c_str());
    }

    static void _add_type_function_to_mt(lua_State *state, const std::string &type_name, const BoundFunctionDef &fn,
            bool is_const) {
        luaL_getmetatable(state, ((is_const ? k_const_prefix : "") + type_name).c_str());

        if (fn.type == FunctionType::MemberInstance || fn.type == FunctionType::Extension) {
            // get the dispatch table for the type
            lua_getmetatable(state, -1);

            _bind_fn(state, fn, type_name);

            // pop the dispatch table and metatable
            lua_pop(state, 2);
        } else {
            _bind_fn(state, fn, type_name);
            // pop the metatable
            lua_pop(state, 1);
        }
    }

    static void _bind_type_function(lua_State *state, const std::string &type_name, const BoundFunctionDef &fn) {
        _add_type_function_to_mt(state, type_name, fn, false);
        _add_type_function_to_mt(state, type_name, fn, true);
    }

    static void _bind_type_field(lua_State *state, const std::string &type_name, const BoundFieldDef &field) {
        UNUSED(state);
        UNUSED(type_name);
        UNUSED(field);
        //TODO
    }

    static void _create_type_metatable(lua_State *state, const BoundTypeDef &type, bool is_const) {
        // create metatable for type
        luaL_newmetatable(state, ((is_const ? k_const_prefix : "") + type.name).c_str());

        // create dispatch table
        lua_newtable(state);

        // bind __index and __newindex overrides

        // push __index function to stack
        lua_pushcfunction(state, _lua_type_index_handler);
        // save function override
        lua_setfield(state, -3, k_lua_index);

        // push __newindex function to stack
        lua_pushcfunction(state, _lua_type_newindex_handler);
        // save function override
        lua_setfield(state, -3, k_lua_newindex);

        // push clone function to stack
        lua_pushcfunction(state, _clone_object);
        // save function to dispatch table
        lua_setfield(state, -2, k_clone_fn);

        // save dispatch table (which pops it from the stack)
        lua_setmetatable(state, -2);

        if (!is_const) {
            // add metatable to global state to provide access to static type functions (popping it from the stack)
            lua_setglobal(state, type.name.c_str());
        } else {
            // don't bother binding const version by name
            lua_pop(state, 1);
        }
    }

    static void _bind_type(lua_State *state, const BoundTypeDef &type) {
        _create_type_metatable(state, type, false);
        _create_type_metatable(state, type, true);

        for (auto &[_, field] : type.fields) {
            _bind_type_field(state, type.name, field);
        }

        for (auto &[_, fn] : type.static_functions) {
            _bind_type_function(state, type.name, fn);
        }

        for (auto &[_, fn] : type.instance_functions) {
            _bind_type_function(state, type.name, fn);
        }

        for (auto &[_, fn] : type.extension_functions) {
            _bind_type_function(state, type.name, fn);
        }
    }

    static void _bind_global_fn(lua_State *state, const BoundFunctionDef &fn) {
        argus_assert(fn.type == FunctionType::Global);

        // put the namespace table on the stack
        luaL_getmetatable(state, k_engine_namespace);
        _bind_fn(state, fn, "");
        // pop the namespace table
        lua_pop(state, 1);
    }

    static void _bind_enum(lua_State *state, const BoundEnumDef &def) {
        // create metatable for enum
        luaL_newmetatable(state, def.name.c_str());

        // set values in metatable
        for (const auto &[val_name, val] : def.values) {
            lua_pushinteger(state, *reinterpret_cast<const int64_t *>(&val));
            lua_setfield(state, -2, val_name.c_str());
        }

        // add metatable to global state to make enum available
        luaL_getmetatable(state, def.name.c_str());
        lua_setglobal(state, def.name.c_str());

        // pop the metatable
        lua_pop(state, 1);
    }

    static std::string _convert_path_to_uid(const std::string &path) {
        if (path[0] == '.' || path[path.length() - 1] == '.' || path.find("..") != std::string::npos) {
            Logger::default_logger().warn("Module name '%s' is malformed "
                                          "(assuming it is a resource UID)", path.c_str());
        }

        auto cur_index = path.find('.');
        if (cur_index == std::string::npos) {
            Logger::default_logger().warn("Module name '%s' does not include a namespace "
                                          "(assuming it is a resource UID)", path.c_str());
            return "";
        }

        std::string uid = path.substr(0, cur_index) + ":";

        auto last_index = cur_index;
        while ((cur_index = path.find('.', last_index + 1)) != std::string::npos) {
            uid += path.substr(last_index + 1, cur_index - (last_index + 1)) + "/";
            last_index = cur_index;
        }
        uid += path.substr(last_index + 1);

        return uid;
    }

    [[nodiscard]] static Result<int, ScriptLoadError> _load_script(lua_State *state, const Resource &resource) {
        auto &loaded_script = resource.get<LoadedScript>();

        auto load_res = luaL_loadbuffer(state, loaded_script.source.c_str(), loaded_script.source.length(),
                resource.prototype.uid.c_str());
        if (load_res != LUA_OK) {
            const char *err_msg = lua_tostring(state, -1);
            auto uid = resource.prototype.uid;
            resource.release();
            return err<int, ScriptLoadError>(uid, "Failed to parse script " + uid
                    + " (" + std::string(err_msg) + ")");
        }

        auto call_res = lua_pcall(state, 0, 1, 0);
        if (call_res != LUA_OK) {
            //TODO: print detailed trace info from VM
            auto uid = resource.prototype.uid;
            resource.release();
            return err<int, ScriptLoadError>(uid, lua_tostring(state, -1));
        }

        return ok<int, ScriptLoadError>(1);
    }

    static int _require_override(lua_State *state) {
        const char *path = lua_tostring(state, 1);
        if (path == nullptr) {
            return luaL_error(state, "Incorrect arguments to function 'require'");
        }

        auto uid = _convert_path_to_uid(path);
        if (!uid.empty()) {
            auto load_res = ScriptManager::instance().load_resource(k_plugin_lang_name, uid);
            if (load_res.is_ok()) {
                auto load_script_res = _load_script(state, load_res.unwrap());
                if (load_script_res.is_ok()) {
                    return load_script_res.unwrap();
                } else {
                    return luaL_error(state, "Unable to parse script %s passed to 'require': %s",
                            path, load_script_res.unwrap_err().msg.c_str());
                }
            } else {
                Logger::default_logger().debug("Unable to load resource for require path %s (%s)",
                        path, load_res.unwrap_err().msg.c_str());
                // swallow
            }
        }

        Logger::default_logger().warn("Unable to load Lua module '%s' as resource; "
                                      "falling back to default require behavior", path);

        // If load_script failed, fall back to old require
        lua_getglobal(state, k_lua_require_def);
        lua_pushstring(state, path);
        if (lua_pcall(state, 0, 1, 0) != 0) {
            return luaL_error(state, "Error executing function 'require': %s", lua_tostring(state, -1));
        }

        return 1;
    }

    LuaLanguagePlugin::LuaLanguagePlugin(void) {
    }

    LuaLanguagePlugin::~LuaLanguagePlugin(void) = default;

    const char *LuaLanguagePlugin::get_language_name(void) const {
        return k_plugin_lang_name;
    }

    const std::vector<std::string> &LuaLanguagePlugin::get_media_types(void) const {
        return k_plugin_resource_types;
    }

    void *LuaLanguagePlugin::create_context_data() {
        // Lua state is implicitly created by LuaContextData's
        // ManagedLuaState member
        auto *data = new LuaContextData(*this);

        // override require behavior
        lua_getglobal(*data->m_state, k_lua_require);
        lua_setglobal(*data->m_state, k_lua_require_def);

        lua_pushcfunction(*data->m_state, _require_override);
        lua_setglobal(*data->m_state, k_lua_require);

        // create namespace table
        luaL_newmetatable(*data->m_state, k_engine_namespace);
        lua_setglobal(*data->m_state, k_engine_namespace);

        return data;
    }

    void LuaLanguagePlugin::destroy_context_data(void *data) {
        auto *lua_data = reinterpret_cast<LuaContextData *>(data);
        // Lua state is implicitly destroyed when LuaContextData's
        // ManagedLuaState member is destructed
        delete lua_data;
    }

    Result<void, ScriptLoadError> LuaLanguagePlugin::load_script(ScriptContext &context, const Resource &resource) {
        argus_assert(resource.prototype.media_type == k_resource_type_lua);

        auto *plugin_data = context.get_plugin_data<LuaContextData>();
        auto &state = plugin_data->m_state;

        auto &loaded_script = resource.get<LoadedScript>();

        auto load_res = luaL_loadbuffer(*state, loaded_script.source.c_str(), loaded_script.source.length(),
                resource.prototype.uid.c_str());
        if (load_res != LUA_OK) {
            const char *err_msg = lua_tostring(*state, -1);
            return err<void, ScriptLoadError>(resource.prototype.uid, "Failed to parse script " + resource.prototype.uid
                    + " (" + std::string(err_msg) + ")");
        }

        auto call_res = lua_pcall(*state, 0, 0, 0);
        if (call_res != LUA_OK) {
            //TODO: print detailed trace info from VM
            return err<void, ScriptLoadError>(resource.prototype.uid, lua_tostring(*state, -1));
        }

        return ok<void, ScriptLoadError>();
    }

    void LuaLanguagePlugin::bind_type(ScriptContext &context, const BoundTypeDef &type) {
        auto *plugin_state = context.get_plugin_data<LuaContextData>();
        auto &state = plugin_state->m_state;
        StackGuard stack_guard(*state);

        _bind_type(*state, type);
    }

    void LuaLanguagePlugin::bind_global_function(ScriptContext &context, const BoundFunctionDef &fn) {
        auto *plugin_state = context.get_plugin_data<LuaContextData>();
        auto &state = plugin_state->m_state;
        StackGuard stack_guard(*state);

        _bind_global_fn(*state, fn);
    }

    void LuaLanguagePlugin::bind_enum(ScriptContext &context, const BoundEnumDef &enum_def) {
        auto *plugin_state = context.get_plugin_data<LuaContextData>();
        auto &state = plugin_state->m_state;
        StackGuard stack_guard(*state);

        _bind_enum(*state, enum_def);
    }

    Result<ObjectWrapper, ScriptInvocationError> LuaLanguagePlugin::invoke_script_function(ScriptContext &context,
            const std::string &name, const std::vector<ObjectWrapper *> &params) {
        if (params.size() > INT32_MAX) {
            return err<ObjectWrapper, ScriptInvocationError>(name, "Too many params");
        }

        auto *plugin_state = context.get_plugin_data<LuaContextData>();
        auto &state = plugin_state->m_state;
        StackGuard stack_guard(*state);

        lua_getglobal(*state, name.c_str());

        auto retval = _invoke_lua_function(*state, params, name);
        if (retval.is_ok) {
            return ok<ObjectWrapper, ScriptInvocationError>(retval.value.value());
        } else {
            return err<ObjectWrapper, ScriptInvocationError>(retval.error.value());
        }
    }
}
