// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"

// module lowlevel
#include "internal/logging.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/texture_loader.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_video.h>

namespace argus {

    bool g_renderer_initialized = false;

    std::vector<Window*> g_windows;
    size_t g_window_count = 0;

    static void _clean_up(void) {
        // use a copy since Window::destroy modifies the global list
        auto windows_copy = g_windows;
        // doing this in reverse ensures that child windows are destroyed before their parents
        for (std::vector<Window*>::reverse_iterator it = windows_copy.rbegin();
                it != windows_copy.rend(); it++) { 
            (*it)->destroy();
        }

        SDL_VideoQuit();

        return;
    }

    static void _init_opengl(void) {
        int context_flags = 0;
        #ifdef _ARGUS_DEBUG_MODE
        context_flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
        #endif

        #ifdef USE_GLES
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        #else
        context_flags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        #endif

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1); 

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);

        init_opengl_extensions();
    }

    void update_lifecycle_renderer(LifecycleStage stage) {
        if (stage == LifecycleStage::INIT) {
            if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
                _ARGUS_FATAL("Failed to initialize SDL video\n");
            }
            _init_opengl();

            g_renderer_initialized = true;
        } else if (stage == LifecycleStage::LATE_INIT) {
            ResourceManager::get_global_resource_manager().register_loader(RESOURCE_TYPE_TEXTURE_PNG, new PngTextureLoader());
        } else if (stage == LifecycleStage::DEINIT) {
            _clean_up();
        }
    }

}
