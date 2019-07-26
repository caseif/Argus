#pragma once

#include <SDL2/SDL_opengl.h>

namespace argus {

    static void (*glGenFramebuffers)(GLsizei n, GLuint* framebuffers);
    static void (*glFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);

    void load_opengl_extensions(void);

}
