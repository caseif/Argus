#define GL_EXT

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

namespace argus {

    RenderLayer::RenderLayer(void) {
        // init the framebuffer
        glGenFramebuffers(1, &framebuffer);

        // init the texture
        glGenTextures(1, &gl_texture);
        glBindTexture(GL_TEXTURE_2D, gl_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

}
