// module lowlevel
#include "internal/logging.hpp"

// module core
#include "internal/util.hpp"

// module renderer
#include "internal/expand_dong.hpp"
#include "internal/glext.hpp"

#include <map>

#define EXPAND_GL_DEFINITION(function) PTR_##function function;
#define EXPAND_GL_INIT_GLOBAL(function) _init_gl_ptr<__COUNTER__>(#function, &function);
#define EXPAND_GL_INIT_SCOPED(function) _load_gl_ext(#function, &funcs_struct.function);

namespace argus {

    namespace glext {
        EXPAND_LIST(EXPAND_GL_DEFINITION, GL_FUNCTIONS);

        // I can't think of a way to achieve this end without declaration duplication
        struct GLExtFuncs {
            EXPAND_LIST(EXPAND_GL_DEFINITION, GL_FUNCTIONS);
        };
    }

    template <typename FunctionSpec>
    static void _load_gl_ext(const char *const func_name, FunctionSpec *target) {
        //TODO: verify the extension for each given function is supported
        void *function = SDL_GL_GetProcAddress(func_name);
        const char* error = SDL_GetError();
        if (error[0] != '\0') {
            _ARGUS_FATAL("Failed to get address for GL function %s: %s\n", func_name, error);
		}
        SDL_ClearError();

        if (!function) {
            _ARGUS_FATAL("Failed to load OpenGL extension: %s\n", func_name);
        }

        *target = reinterpret_cast<FunctionSpec>(function);
    }

    #ifdef _WIN32
    static std::map<SDL_GLContext, struct glext::GLExtFuncs> g_per_context_regs;

    void load_gl_extensions_for_current_context() {
        SDL_GLContext ctx = SDL_GL_GetCurrentContext();
        _ARGUS_ASSERT(ctx, "No context is current\n");

        struct glext::GLExtFuncs funcs_struct;
        EXPAND_LIST(EXPAND_GL_INIT_SCOPED, GL_FUNCTIONS);
        
        g_per_context_regs.insert({ctx, funcs_struct});
    }

    template <size_t FunctionIndex, typename RetType, typename... ParamTypes>
    static RetType APIENTRY _gl_trampoline(ParamTypes... params) {
        struct glext::GLExtFuncs *funcStruct;

        SDL_GLContext gl_ctx = SDL_GL_GetCurrentContext();
        if (!gl_ctx) {
            _ARGUS_FATAL("No GL context is current\n");
        }

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
		#ifndef _WIN32
		if (SDL_GL_LoadLibrary(nullptr) != 0) {
			_ARGUS_FATAL("Failed to load GL library\n");
		}
		#endif

        using namespace glext;

        EXPAND_LIST(EXPAND_GL_INIT_GLOBAL, GL_FUNCTIONS);
    }

}
