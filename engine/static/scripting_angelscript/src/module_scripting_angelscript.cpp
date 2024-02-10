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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"

#include "argus/core/module.hpp"

#include "argus/resman.hpp"

#include "internal/scripting_angelscript/angelscript_loader.hpp"
#include "internal/scripting_angelscript/angelscript_proxy.hpp"
#include "internal/scripting_angelscript/module_scripting_angelscript.hpp"

#include <cstdio>

namespace argus {
    asIScriptEngine *g_as_script_engine;

    static void _script_engine_message_callback(const asSMessageInfo *msg, void *param) {
        UNUSED(param);

        constexpr const char *format = "[AngelScript] %s (%d, %d) : %s\n";
        switch (msg->type) {
            case asMSGTYPE_ERROR:
                Logger::default_logger().severe(format, msg->section, msg->row, msg->col, msg->message);
                break;
            case asMSGTYPE_WARNING:
                Logger::default_logger().warn(format, msg->section, msg->row, msg->col, msg->message);
                break;
            case asMSGTYPE_INFORMATION:
                Logger::default_logger().info(format, msg->section, msg->row, msg->col, msg->message);
                break;
        }
    }

    /*static void print(const std::string &msg) {
        printf("%s\n", msg.c_str());
    }*/

    static void _register_builtin_functions() {
        /*affirm_precond(register_global_function("void println(const string &in)", reinterpret_cast<void *>(&print)),
                "Failed to register println function");*/
    }

    static void _setup_script_engine(void) {
        g_as_script_engine = asCreateScriptEngine();
        g_as_script_engine->SetMessageCallback(asFUNCTION(_script_engine_message_callback), nullptr, asCALL_CDECL);

        RegisterStdString(g_as_script_engine);

        _register_builtin_functions();
    }

    extern "C" void update_lifecycle_scripting_angelscript(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PreInit:
                ResourceManager::instance().register_loader(*new AngelscriptLoader());
                break;
            case LifecycleStage::Init:
                _setup_script_engine();
                break;
            default:
                break;
        }
    }
}
