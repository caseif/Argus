#pragma once

#include <SDL2/SDL_opengl.h>

namespace argus {

    static void (*glGenFramebuffers)(GLsizei, GLuint*);

    void load_opengl_extensions(void);

}
