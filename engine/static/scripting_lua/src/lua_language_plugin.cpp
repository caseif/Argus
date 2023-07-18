/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/logging.hpp"

#include "argus/resman.hpp"

#include "argus/scripting/bridge.hpp"
#include "argus/scripting/exception.hpp"
#include "argus/scripting/scripting_language_plugin.hpp"
#include "argus/scripting/util.hpp"

#include "internal/scripting_lua/loaded_script.hpp"
#include "internal/scripting_lua/lua_context_data.hpp"
#include "internal/scripting_lua/lua_language_plugin.hpp"
#include "internal/scripting_lua/module_scripting_lua.hpp"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include "internal/scripting_lua/defines.hpp"

#include <functional>
#include <memory>
#include <string>

#include <cassert>
#include <cstdio>

namespace argus {
    static constexpr const char *k_lang_name = "lua";

    static constexpr const char *k_lua_index = "__index";
    static constexpr const char *k_lua_name = "__name";
    static constexpr const char *k_lua_require = "require";
    static constexpr const char *k_lua_require_def = "default_require";

    struct UserData {
        bool is_pointer;
        char data[0];
    };

    static ObjectWrapper _invoke_lua_function(lua_State *state, const std::vector<ObjectWrapper> &params,
            const std::optional<std::string> &fn_name = std::nullopt);

    struct LuaCallback {
        lua_State *state;
        int ref_key;

        LuaCallback(lua_State *state) : state(state) {
            // duplicate the top stack value in order to leave the stack as we
            // found it
            lua_pushvalue(state, -1);
            ref_key = luaL_ref(state, LUA_REGISTRYINDEX);
        }

        LuaCallback(const LuaCallback &rhs) = delete;

        ~LuaCallback() {
            if (state == nullptr) {
                return;
            }

            luaL_unref(state, LUA_REGISTRYINDEX, ref_key);
        }

        [[nodiscard]] ObjectWrapper call(const std::vector<ObjectWrapper> &params) const {
            auto initial_top = lua_gettop(state);

            lua_rawgeti(state, LUA_REGISTRYINDEX, ref_key);

            auto retval = _invoke_lua_function(state, params);

            assert(lua_gettop(state) == initial_top);
            return retval;
        }
    };

    static int _set_lua_error(lua_State *state, const std::string &msg) {
        return luaL_error(state, msg.c_str());
    }

    static int _wrap_instance_ref(lua_State *state, const std::string &qual_fn_name,
            int param_index, const BoundTypeDef &type_def, ObjectWrapper *dest) {
        if (!lua_isuserdata(state, param_index)
            || !luaL_testudata(state, param_index, type_def.name.c_str())) {
            return _set_lua_error(state, "Incorrect type provided for parameter "
                                  + std::to_string(param_index) + " of function " + qual_fn_name
                                  + " (expected " + type_def.name
                                  + ", actual " + luaL_typename(state, param_index) + ")");
        }

        auto *udata = reinterpret_cast<UserData *>(lua_touserdata(state, param_index));
        void *ptr;
        if (udata->is_pointer) {
            ptr = *reinterpret_cast<void **>(udata->data);
        } else {
            ptr = static_cast<void *>(udata->data);
        }

        ObjectType obj_type{};
        obj_type.type = IntegralType::Pointer;
        obj_type.type_name = type_def.name;
        obj_type.size = sizeof(void *);
        *dest = create_object_wrapper(obj_type, ptr);
        return 0;
    }

    static int _wrap_param(lua_State *state, const std::string &qual_fn_name,
            int param_index, const ObjectType &param_def, ObjectWrapper *dest) {
        switch (param_def.type) {
            case Integer:
            case Enum: {
                if (!lua_isinteger(state, param_index)) {
                    return _set_lua_error(state, "Incorrect type provided for parameter "
                                          + std::to_string(param_index) + " of function " + qual_fn_name
                                          + " (expected integer "
                                          + (param_def.type == IntegralType::Enum ? "(enum) " : "")
                                          + ", actual " + luaL_typename(state, param_index) + ")");
                }

                int64_t val_i64 = lua_tointeger(state, param_index);

                if (param_def.type == IntegralType::Enum) {
                    assert(param_def.type_name.has_value());
                    auto &enum_def = get_bound_enum(param_def.type_name.value());
                    auto enum_val_it = enum_def.all_ordinals.find(*reinterpret_cast<uint64_t *>(&val_i64));
                    if (enum_val_it == enum_def.all_ordinals.cend()) {
                        return _set_lua_error(state, "Unknown ordinal " + std::to_string(val_i64) + " provided for enum "
                                + enum_def.name + " at parameter " + std::to_string(param_index) + " of function "
                                + qual_fn_name);
                    }
                }

                switch (param_def.size) {
                    case 1: {
                        auto val_i8 = int8_t(val_i64);
                        *dest = create_object_wrapper(param_def, &val_i8);
                        break;
                    }
                    case 2: {
                        auto val_i16 = int16_t(val_i64);
                        *dest = create_object_wrapper(param_def, &val_i16);
                        break;
                    }
                    case 4: {
                        auto val_i32 = int32_t(val_i64);
                        *dest = create_object_wrapper(param_def, &val_i32);
                        break;
                    }
                    case 8: {
                        *dest = create_object_wrapper(param_def, &val_i64);
                        break;
                    }
                    default:
                        assert(false); // should have been caught during binding
                }

                return 0;
            }
            case Float: {
                if (!lua_isnumber(state, param_index)) {
                    return _set_lua_error(state, "Incorrect type provided for parameter "
                                          + std::to_string(param_index) + " of function " + qual_fn_name
                                          + " (expected number, actual "
                                          + luaL_typename(state, param_index) + ")");
                }

                double val_f64 = lua_tonumber(state, param_index);
                if (param_def.size == 8) {
                    // can push double-precision float as-is
                    *dest = create_object_wrapper(param_def, &val_f64);
                } else {
                    assert(param_def.size == 4); // should have been caught during binding
                    // need to reduce precision
                    float val_f32 = float(val_f64);
                    *dest = create_object_wrapper(param_def, &val_f32);
                }

                return 0;
            }
            case String: {
                if (!lua_isstring(state, param_index)) {
                    return _set_lua_error(state, "Incorrect type provided for parameter "
                                          + std::to_string(param_index) + " of function " + qual_fn_name
                                          + " (expected string, actual "
                                          + luaL_typename(state, param_index) + ")");
                }

                const char *str = lua_tostring(state, param_index);
                *dest = create_string_object_wrapper(param_def, std::string(str));

                return 0;
            }
            case Struct:
            case Pointer: {
                assert(param_def.type_name.has_value());
                if (!lua_isuserdata(state, param_index)) {
                    return _set_lua_error(state, "Incorrect type provided for parameter "
                                          + std::to_string(param_index) + " of function " + qual_fn_name
                                          + " (expected userdata, actual "
                                          + luaL_typename(state, param_index) + ")");
                }

                // get metatable of userdata
                lua_getmetatable(state, param_index);

                // get metatable name
                lua_pushstring(state, k_lua_name);
                lua_gettable(state, -2);
                const char *type_name = lua_tostring(state, -1);

                lua_pop(state, 2); // remove field name and metatable from stack

                if (param_def.type_name.value() != type_name) {
                    return _set_lua_error(state, "Incorrect userdata provided for parameter "
                                          + std::to_string(param_index) + " of function " + qual_fn_name
                                          + " (expected " + param_def.type_name.value()
                                          + ", actual " + type_name + ")");
                }

                auto *udata = reinterpret_cast<UserData *>(lua_touserdata(state, param_index));
                void *ptr;
                if (udata->is_pointer) {
                    // userdata is storing pointer to struct data
                    ptr = *reinterpret_cast<void **>(udata->data);
                } else {
                    // userdata is directly storing struct data
                    ptr = static_cast<void *>(udata->data);
                }

                *dest = create_object_wrapper(param_def, ptr);

                return 0;
            }
            case Callback: {
                /*if (!lua_isfunction(state, param_index)) {
                    return _set_lua_error(state, "Incorrect type provided for parameter "
                                                 + std::to_string(param_index) + " of function " + qual_fn_name
                                                 + " (expected function, actual "
                                                 + luaL_typename(state, param_index) + ")");
                }*/

                auto handle = std::make_shared<LuaCallback>(state);

                ProxiedFunction fn = [handle = std::move(handle)](const std::vector<ObjectWrapper> &params) {
                    return handle->call(params);
                };

                *dest = create_callback_object_wrapper(param_def, fn);

                return 0;
            }
            default:
                Logger::default_logger().fatal("Unknown integral type ordinal %d\n", param_def.type);
        }
    }

    static int64_t _unwrap_int_wrapper(ObjectWrapper wrapper) {
        assert(wrapper.type.type == IntegralType::Integer
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
                throw std::invalid_argument("Bad integer width " + std::to_string(wrapper.type.size)
                                            + " (must be 1, 2, 4, or 8)");
        }
    }

    static double _unwrap_float_wrapper(ObjectWrapper wrapper) {
        assert(wrapper.type.type == IntegralType::Float);

        switch (wrapper.type.size) {
            case 4:
                return double(*reinterpret_cast<const float *>(wrapper.value));
            case 8:
                return *reinterpret_cast<const double *>(wrapper.value);
            default:
                throw std::invalid_argument("Bad floating-point width " + std::to_string(wrapper.type.size)
                                            + " (must be 4, or 8)");
        }
    }

    static void _set_metatable(lua_State *state, const ObjectWrapper &wrapper) {
        auto mt = luaL_getmetatable(state, wrapper.type.type_name.value().c_str());
        assert(mt != 0); // binding should have failed if type wasn't bound

        lua_setmetatable(state, -2);
    }

    static void _push_value(lua_State *state, const ObjectWrapper &wrapper) {
        assert(wrapper.type.type != IntegralType::Void);

        switch (wrapper.type.type) {
            case IntegralType::Integer:
            case IntegralType::Enum:
                lua_pushinteger(state, _unwrap_int_wrapper(wrapper));
                break;
            case IntegralType::Float:
                lua_pushnumber(state, _unwrap_float_wrapper(wrapper));
                break;
            case IntegralType::String:
                lua_pushstring(state, reinterpret_cast<const char *>(
                        wrapper.is_on_heap ? wrapper.heap_ptr : wrapper.value));
                break;
            case IntegralType::Struct: {
                assert(wrapper.type.type_name.has_value());

                const void *ptr = wrapper.is_on_heap ? wrapper.heap_ptr : wrapper.value;
                auto *udata = reinterpret_cast<UserData *>(lua_newuserdata(state,
                        sizeof(UserData) + wrapper.type.size));
                udata->is_pointer = false;
                memcpy(udata->data, ptr, wrapper.type.size);
                _set_metatable(state, wrapper);

                break;
            }
            case IntegralType::Pointer: {
                assert(wrapper.type.type_name.has_value());

                void *ptr = wrapper.is_on_heap ? wrapper.heap_ptr : wrapper.stored_ptr;
                auto *udata = reinterpret_cast<UserData *>(lua_newuserdata(state,
                        sizeof(UserData) + sizeof(void *)));
                udata->is_pointer = true;
                memcpy(udata->data, &ptr, sizeof(void *));
                _set_metatable(state, wrapper);

                break;
            }
            default:
                assert(false);
        }
    }

    static ObjectWrapper _invoke_lua_function(lua_State *state, const std::vector<ObjectWrapper> &params,
            const std::optional<std::string> &fn_name) {
        int i = 1;
        try {
            for (const auto &param : params) {
                _push_value(state, param);
                i++;
            }
        } catch (const std::exception &ex) {
            throw ScriptInvocationException(fn_name.value_or("callback"),
                    "Bad value passed for parameter " + std::to_string(i) + ": " + ex.what());
        }

        if (lua_pcall(state, int(params.size()), 0, 0) != LUA_OK) {
            auto err = lua_tostring(state, -1);
            lua_pop(state, 1); // pop error message
            throw ScriptInvocationException(fn_name.value_or("callback"), err);
        }

        return {}; //TODO
    }

    static int _lua_trampoline(lua_State *state) {
        auto initial_top = lua_gettop(state);

        auto fn_type = static_cast<FunctionType>(lua_tointeger(state, lua_upvalueindex(1)));
        if (fn_type != FunctionType::Global && fn_type != FunctionType::MemberInstance
                && fn_type != FunctionType::MemberStatic) {
            Logger::default_logger().fatal("Popped unknown function type value from Lua stack");
        }

        std::string type_name;
        auto fn_name_index = 2;
        if (fn_type != FunctionType::Global) {
            type_name = lua_tostring(state, lua_upvalueindex(2));
            fn_name_index = 3;
        }

        std::string fn_name(lua_tostring(state, lua_upvalueindex(fn_name_index)));

        auto qual_fn_name = get_qualified_function_name(fn_type, type_name, fn_name);

        try {
            BoundFunctionDef fn;
            switch (fn_type) {
                case FunctionType::Global:
                    fn = get_native_global_function(fn_name);
                    break;
                case FunctionType::MemberInstance:
                    fn = get_native_member_instance_function(type_name, fn_name);
                    break;
                case FunctionType::MemberStatic:
                    fn = get_native_member_static_function(type_name, fn_name);
                    break;
                default:
                    Logger::default_logger().fatal("Unknown function type ordinal %d", fn_type);
            }

            // parameter count not including instance
            auto arg_count = initial_top;
            auto expected_arg_count = fn.params.size() + (fn.type == FunctionType::MemberInstance ? 1 : 0);
            if (size_t(arg_count) != expected_arg_count) {
                _set_lua_error(state, "Wrong parameter count provided for function " + qual_fn_name
                        + " (expected " + std::to_string(expected_arg_count)
                        + ", actual " + std::to_string(arg_count) + ")");
                assert(lua_gettop(state) == initial_top);
                return 0;
            }

            // calls to instance member functions push the instance as the first "parameter"
            auto first_param_index = fn_type == FunctionType::MemberInstance ? 1 : 0;

            std::vector<ObjectWrapper> args;

            if (fn_type == FunctionType::MemberInstance) {
                auto type_def = get_bound_type(type_name);
                ObjectType instance_type{};
                instance_type.type = IntegralType::Pointer;
                instance_type.size = type_def.size;
                instance_type.type_name = type_def.name;

                //TODO: add safeguard to prevent invocation of functions on non-references
                ObjectWrapper wrapper{};
                auto wrap_res = _wrap_instance_ref(state, qual_fn_name, 1, type_def, &wrapper);
                if (wrap_res == 0) {
                    args.push_back(wrapper);
                } else {
                    // some error occurred
                    // _wrap_instance_ref already sent error to lua state, so just clean up here
                    cleanup_object_wrappers(args);
                    assert(lua_gettop(state) == initial_top);
                    return wrap_res;
                }
            }

            for (int i = 0; i < (arg_count - first_param_index); i++) {
                // Lua is 1-indexed, plus add offset to skip instance parameter if present
                auto param_index = i + 1 + first_param_index;
                auto param_def = fn.params.at(size_t(i));
                ObjectWrapper wrapper{};
                auto wrap_res = _wrap_param(state, qual_fn_name, param_index, param_def, &wrapper);
                if (wrap_res == 0) {
                    args.push_back(wrapper);
                } else {
                    cleanup_object_wrappers(args);
                    assert(lua_gettop(state) == initial_top);
                    return wrap_res;
                }
            }

            auto retval = fn.handle(args);

            cleanup_object_wrappers(args);

            if (retval.type.type != IntegralType::Void) {
                try {
                    _push_value(state, retval);

                    cleanup_object_wrapper(retval);
                } catch (const std::exception &ex) {
                    Logger::default_logger().fatal("Failed to push return type of bound function to Lua VM");
                }

                assert(lua_gettop(state) == initial_top + 1);
                return 1;
            } else {
                cleanup_object_wrapper(retval);

                assert(lua_gettop(state) == initial_top);
                return 0;
            }
        } catch (const TypeNotBoundException &ex) {
            _set_lua_error(state, "Type with name " + type_name + " is not bound");
            assert(lua_gettop(state) == initial_top);
            return 0;
        } catch (const FunctionNotBoundException &ex) {
            _set_lua_error(state, "Function with name " + qual_fn_name + " is not bound");
            assert(lua_gettop(state) == initial_top);
            return 0;
        } catch (const ReflectiveArgumentsException &ex) {
            _set_lua_error(state, "Bad arguments provided to function " + qual_fn_name + " (" + ex.what() + ")");
            assert(lua_gettop(state) == initial_top);
            return 0;
        }
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

    static int _load_script(lua_State *state, const Resource &resource) {
        auto &loaded_script = resource.get<LoadedScript>();

        if (luaL_loadstring(state, loaded_script.source.c_str()) != LUA_OK) {
            resource.release();
            throw ScriptLoadException(resource.uid, "luaL_loadstring failed");
        }

        auto err = lua_pcall(state, 0, 1, 0);
        if (err != LUA_OK) {
            //TODO: print detailed trace info from VM
            resource.release();
            throw ScriptLoadException(resource.uid, lua_tostring(state, -1));
        }

        return 1;
    }

    static int _require_override(lua_State *state) {
        auto &plugin = *get_plugin_from_state(state);

        const char *path = lua_tostring(state, 1);
        if (path == nullptr) {
            return luaL_error(state, "Incorrect arguments to function 'require'");
        }

        auto uid = _convert_path_to_uid(path);
        if (!uid.empty()) {
            try {
                auto &res = plugin.load_resource(uid);
                return _load_script(state, res);
            } catch (const std::exception &ex) {
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

    static void _bind_fn(lua_State *state, const BoundFunctionDef &fn, const std::string &type_name) {
        // push function type
        lua_pushinteger(state, fn.type);
        // push type name (only if member function)
        if (fn.type != FunctionType::Global) {
            lua_pushstring(state, type_name.c_str());
        }
        // push function name
        lua_pushstring(state, fn.name.c_str());

        auto upvalue_count = fn.type == FunctionType::Global ? 2 : 3;

        lua_pushcclosure(state, _lua_trampoline, upvalue_count);

        if (fn.type == FunctionType::Global) {
            lua_setglobal(state, fn.name.c_str());
        } else {
            lua_setfield(state, -2, fn.name.c_str());
        }
    }

    static void _bind_type(lua_State *state, const BoundTypeDef &type) {
        // create metatable for type
        luaL_newmetatable(state, type.name.c_str());

        // create dispatch table
        lua_newtable(state);

        // set dispatch table (which pops it from the stack)
        lua_setfield(state, -2, k_lua_index);

        // add metatable to global state to provide access to static type functions
        luaL_getmetatable(state, type.name.c_str());
        lua_setglobal(state, type.name.c_str());
        lua_pop(state, 1); // remove metatable from stack
    }

    static void _bind_type_function(lua_State *state, const std::string &type_name, const BoundFunctionDef &fn) {
        luaL_getmetatable(state, (type_name).c_str());

        if (fn.type == FunctionType::MemberInstance) {
            // get the dispatch table for the type
            lua_getfield(state, -1, k_lua_index);

            _bind_fn(state, fn, type_name);

            // pop the dispatch table and metatable
            lua_pop(state, 2);

            return;
        } else {
            _bind_fn(state, fn, type_name);

            // pop the metatable
            lua_pop(state, 1);
        }
    }

    static void _bind_global_fn(lua_State *state, const BoundFunctionDef &fn) {
        assert(fn.type == FunctionType::Global);
        _bind_fn(state, fn, "");
    }

    static void _bind_enum(lua_State *state, const BoundEnumDef &def) {
        // create metatable for enum
        luaL_newmetatable(state, def.name.c_str());

        // set values in metatable
        for (const auto &enum_val : def.values) {
            lua_pushinteger(state, *reinterpret_cast<const int64_t *>(&enum_val.second));
            lua_setfield(state, -2, enum_val.first.c_str());
        }

        // add metatable to global state to make enum available
        luaL_getmetatable(state, def.name.c_str());
        lua_setglobal(state, def.name.c_str());

        // pop the metatable
        lua_pop(state, 1);
    }

    LuaLanguagePlugin::LuaLanguagePlugin(void) : ScriptingLanguagePlugin(k_lang_name, { RESOURCE_TYPE_LUA }) {
    }

    LuaLanguagePlugin::~LuaLanguagePlugin(void) = default;

    void *LuaLanguagePlugin::create_context_data() {
        auto *data = new LuaContextData();
        data->state = create_lua_state(*this);

        // override require behavior
        lua_getglobal(data->state, k_lua_require);
        lua_setglobal(data->state, k_lua_require_def);

        lua_pushcfunction(data->state, _require_override);
        lua_setglobal(data->state, k_lua_require);

        return data;
    }

    void LuaLanguagePlugin::destroy_context_data(void *data) {
        auto *lua_data = reinterpret_cast<LuaContextData *>(data);

        destroy_lua_state(lua_data->state);

        delete lua_data;
    }

    void LuaLanguagePlugin::load_script(ScriptContext &context, const Resource &resource) {
        assert(resource.prototype.media_type == RESOURCE_TYPE_LUA);

        auto *plugin_data = context.get_plugin_data<LuaContextData>();

        auto *state = plugin_data->state;

        auto &loaded_script = resource.get<LoadedScript>();

        if (luaL_loadstring(state, loaded_script.source.c_str()) != LUA_OK) {
            throw ScriptLoadException(resource.uid, "luaL_loadstring failed");
        }

        auto err = lua_pcall(state, 0, 0, 0);
        if (err != LUA_OK) {
            //TODO: print detailed trace info from VM
            throw ScriptLoadException(resource.uid, lua_tostring(state, -1));
        }
    }

    void LuaLanguagePlugin::bind_type(ScriptContext &context, const BoundTypeDef &type) {
        auto *plugin_state = context.get_plugin_data<LuaContextData>();
        auto *state = plugin_state->state;
        auto initial_top = lua_gettop(state);

        _bind_type(state, type);

        assert(lua_gettop(state) == initial_top);
    }

    void LuaLanguagePlugin::bind_type_function(ScriptContext &context, const BoundTypeDef &type,
            const BoundFunctionDef &fn) {
        auto *plugin_state = context.get_plugin_data<LuaContextData>();
        auto *state = plugin_state->state;
        auto initial_top = lua_gettop(state);

        _bind_type_function(state, type.name, fn);

        assert(lua_gettop(state) == initial_top);
    }

    void LuaLanguagePlugin::bind_global_function(ScriptContext &context, const BoundFunctionDef &fn) {
        auto *plugin_state = context.get_plugin_data<LuaContextData>();
        auto *state = plugin_state->state;
        auto initial_top = lua_gettop(state);

        _bind_global_fn(state, fn);

        assert(lua_gettop(state) == initial_top);
    }

    void LuaLanguagePlugin::bind_enum(ScriptContext &context, const BoundEnumDef &enum_def) {
        auto *plugin_state = context.get_plugin_data<LuaContextData>();
        auto *state = plugin_state->state;
        auto initial_top = lua_gettop(state);

        _bind_enum(state, enum_def);

        assert(lua_gettop(state) == initial_top);
    }

    ObjectWrapper LuaLanguagePlugin::invoke_script_function(ScriptContext &context, const std::string &name,
            const std::vector<ObjectWrapper> &params) {
        if (params.size() > INT32_MAX) {
            throw ScriptInvocationException(name, "Too many params");
        }

        auto *plugin_state = context.get_plugin_data<LuaContextData>();
        auto *state = plugin_state->state;
        auto initial_top = lua_gettop(state);

        lua_getglobal(state, name.c_str());

        try {
            auto retval = _invoke_lua_function(state, params, name);

            assert(lua_gettop(state) == initial_top);

            return retval;
        } catch (const InvocationException &ex) {
            assert(lua_gettop(state) == initial_top);
            throw ex;
        }
    }
}
