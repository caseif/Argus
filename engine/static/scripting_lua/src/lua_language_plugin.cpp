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

#include "argus/scripting/bridge.hpp"
#include "argus/scripting/exception.hpp"
#include "argus/scripting/scripting_language_plugin.hpp"
#include "argus/scripting/util.hpp"

#include "internal/scripting_lua/lua_language_plugin.hpp"
#include "internal/scripting_lua/module_scripting_lua.hpp"

#include "lua.h"
#include "lauxlib.h"

#include <functional>
#include <string>

#include <cassert>

namespace argus {
    static constexpr const char *k_lang_name = "lua";

    static void _set_lua_error(lua_State *state, const std::string &msg) {
        luaL_error(state, msg.c_str());
    }

    static bool _wrap_param(lua_State *state, const std::string &qual_fn_name,
            int param_index, const ObjectType &param_def, ObjectWrapper *dest) {
        auto canonical_index = param_index + 1;
        auto real_index = param_index + 2;

        switch (param_def.type) {
            case Integer: {
                if (!lua_isinteger(state, real_index)) {
                    _set_lua_error(state, "Incorrect type provided for parameter "
                                          + std::to_string(canonical_index) + " of function " + qual_fn_name
                                          + " (expected integer)");
                    return false;
                }

                int64_t val_i64 = lua_tointeger(state, real_index);

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

                return true;
            }
            case Float: {
                if (!lua_isnumber(state, real_index)) {
                    _set_lua_error(state, "Incorrect type provided for parameter "
                                          + std::to_string(canonical_index) + " of function " + qual_fn_name
                                          + " (expected number)");
                    return false;
                }

                double val_f64 = lua_tonumber(state, real_index);
                if (param_def.size == 8) {
                    // can push double-precision float as-is
                    *dest = create_object_wrapper(param_def, &val_f64);
                } else {
                    assert(param_def.size == 4); // should have been caught during binding
                    // need to reduce precision
                    float val_f32 = float(val_f64);
                    *dest = create_object_wrapper(param_def, &val_f32);
                }

                return true;
            }
            case String: {
                if (!lua_isstring(state, real_index)) {
                    _set_lua_error(state, "Incorrect type provided for parameter "
                                          + std::to_string(canonical_index) + " of function " + qual_fn_name
                                          + " (expected string)");
                    return false;
                }

                const char *str = lua_tostring(state, real_index);
                *dest = create_object_wrapper(param_def, std::string(str));

                return true;
            }
            case Opaque: {
                if (!lua_isuserdata(state, real_index)
                        || !luaL_testudata(state, real_index, param_def.type_name.c_str())) {
                    _set_lua_error(state, "Incorrect type provided for parameter "
                                          + std::to_string(canonical_index) + " of function " + qual_fn_name
                                          + " (expected " + param_def.type_name + ")");
                    return false;
                }

                void *ptr = lua_touserdata(state, real_index);

                *dest = create_object_wrapper(param_def, &ptr);

                return true;
            }
            default:
                Logger::default_logger().fatal("Unknown integral type ordinal %d\n", param_def.type);
        }
    }

    static int _lua_trampoline(lua_State *state) {
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

            auto arg_count = lua_gettop(state) - 1;
            if (size_t(arg_count) != fn.params.size()) {
                _set_lua_error(state, "Wrong parameter count provided for function " + qual_fn_name);
                return 0;
            }

            std::vector<ObjectWrapper> args;

            for (int i = 0; i < arg_count; i++) {
                auto param_def = fn.params.at(size_t(i));
                ObjectWrapper wrapper{};
                if (_wrap_param(state, qual_fn_name, i, param_def, &wrapper)) {
                    args.push_back(wrapper);
                } else {
                    cleanup_object_wrappers(args);
                    return 0;
                }
            }

            auto retval = fn.handle(args);
            UNUSED(retval); //TODO

            return 1;
        } catch (const TypeNotBoundException &ex) {
            _set_lua_error(state, "Type with name " + type_name + " is not bound");
            return 0;
        } catch (const FunctionNotBoundException &ex) {
            _set_lua_error(state, "Function with name " + qual_fn_name + " is not bound");
            return 0;
        } catch (const ReflectiveArgumentsException &ex) {
            _set_lua_error(state, "Bad arguments provided to function " + qual_fn_name + " (" + ex.what() + ")");
            return 0;
        }
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
        lua_newtable(state);

        // register instance functions to metatable
        lua_newtable(state);

        for (const auto &fn : type.instance_functions) {
            assert(fn.second.type == FunctionType::MemberInstance);
            _bind_fn(state, fn.second, type.name);
        }

        lua_setfield(state, -2, "__index");

        // register static functions to outer table

        for (const auto &fn : type.static_functions) {
            assert(fn.second.type == FunctionType::MemberStatic);
            _bind_fn(state, fn.second, type.name);
        }
    }

    static void _bind_global_fn(lua_State *state, const BoundFunctionDef &fn) {
        assert(fn.type == FunctionType::Global);
        _bind_fn(state, fn, "");
    }

    LuaLanguagePlugin::LuaLanguagePlugin(void) : ScriptingLanguagePlugin(k_lang_name) {
    }

    LuaLanguagePlugin::~LuaLanguagePlugin(void) = default;

    void LuaLanguagePlugin::bind_type(const BoundTypeDef &type) {
        for (auto *state : g_lua_states) {
            _bind_type(state, type);
        }
    }

    void LuaLanguagePlugin::bind_global_function(const BoundFunctionDef &fn) {
        for (auto *state : g_lua_states) {
            _bind_global_fn(state, fn);
        }
    }

    ObjectWrapper LuaLanguagePlugin::invoke_script_function(const std::string &name,
            const std::vector<ObjectWrapper> &params) {
        UNUSED(name);
        UNUSED(params);
        //TODO
        return {};
    }
}
