/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module render
#include "internal/render/expansion_macros.hpp"
#include "internal/render/glext.hpp"

#include <GLFW/glfw3.h>

#include <map> // IWYU pragma: keep

#include <cstddef>

#define EXPAND_GL_DEFINITION(function) PTR_##function function;
#define EXPAND_GL_INIT_GLOBAL(function) _init_gl_ptr<__COUNTER__>(#function, &function);
#define EXPAND_GL_INIT_SCOPED(function) _load_gl_ext(#function, &funcs_struct.function);

namespace argus {

    static bool is_initialized = false;

    namespace glext {
        EXPAND_LIST(EXPAND_GL_DEFINITION, GL_FUNCTIONS);

        // I can't think of a way to achieve this end without declaration duplication
        #pragma pack(push,1)
        struct GLExtFuncs {
            EXPAND_LIST(EXPAND_GL_DEFINITION, GL_FUNCTIONS);
        };
        #pragma pack(pop)
    }

    template <typename FunctionSpec>
    static void _load_gl_ext(const char *const func_name, FunctionSpec *target) {
        _ARGUS_ASSERT(glfwGetCurrentContext() != nullptr, "No GL context is current\n");
        //TODO: verify the extension for each given function is supported
        GLFWglproc function = glfwGetProcAddress(func_name);
        if (function == nullptr) {
            _ARGUS_FATAL("Failed to get address for GL function %s\n", func_name);
		}

        *target = reinterpret_cast<FunctionSpec>(function);
    }

    #ifdef _WIN32
    static std::map<GLFWwindow*, struct glext::GLExtFuncs> g_per_context_regs;

    void load_gl_extensions_for_current_context() {
        GLFWwindow *ctx = glfwGetCurrentContext();
        _ARGUS_ASSERT(ctx != nullptr, "No GL context is current\n");

        struct glext::GLExtFuncs funcs_struct;
        EXPAND_LIST(EXPAND_GL_INIT_SCOPED, GL_FUNCTIONS);

        g_per_context_regs.insert({ctx, funcs_struct});
    }

    template <size_t FunctionIndex, typename RetType, typename... ParamTypes>
    static RetType APIENTRY _gl_trampoline(ParamTypes... params) {
        struct glext::GLExtFuncs *funcStruct;

        GLFWwindow *gl_ctx = glfwGetCurrentContext();
        _ARGUS_ASSERT(gl_ctx != nullptr, "No GL context is current\n");

        auto it = g_per_context_regs.find(gl_ctx);
        if (it == g_per_context_regs.end()) {
            _ARGUS_FATAL("GL functions are not registered for this context\n");
        }

        funcStruct = &it->second;

        void *ptr = *reinterpret_cast<void**>((reinterpret_cast<unsigned char*>(funcStruct)) + (FunctionIndex * sizeof(void*)));
        return (*reinterpret_cast<RetType(APIENTRY**)(ParamTypes...)>(reinterpret_cast<unsigned char*>(funcStruct) + (FunctionIndex * sizeof(void*))))(params...);
    }
    #endif

    template <size_t FunctionIndex, typename RetType, typename... ParamTypes>
    static void _init_gl_ptr(const char *const func_name, RetType(APIENTRY **target)(ParamTypes...)) {
        #ifdef _WIN32
        // The use of __COUNTER__ here is a horrible hack that exploits the fact
        // that the trampoline functions are initialized in the same order as
        // the GLExtFuncs member definitions. It allows us to use the same
        // _init_gl_ptr calls across platforms.
        *target = reinterpret_cast<RetType(APIENTRY*)(ParamTypes...)>(_gl_trampoline<FunctionIndex, RetType, ParamTypes...>);
        #else
        _load_gl_ext(func_name, target);
        #endif
    }

    void init_opengl_extensions(void) {
        //TODO: stopgap until I figure out a better way to deal with GLFW
        if (is_initialized) {
            return;
        }
        is_initialized = true;

        using namespace glext;

        #ifdef _WIN32
        load_gl_extensions_for_current_context();
        #endif
        
        EXPAND_LIST(EXPAND_GL_INIT_GLOBAL, GL_FUNCTIONS);
    }

}
