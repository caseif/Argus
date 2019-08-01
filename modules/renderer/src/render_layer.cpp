#define GL_EXT

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

namespace argus {

    using glext::glGenFramebuffers;
    using glext::glFramebufferTexture;

    RenderLayer::RenderLayer(Renderer *parent) {
        this->parent_renderer = parent;

        this->root_item = new RenderItem(this, nullptr);

        parent->activate_gl_context();

        // init the framebuffer
        glGenFramebuffers(1, &framebuffer);

        // init the texture
        glGenTextures(1, &gl_texture);
        glBindTexture(GL_TEXTURE_2D, gl_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gl_texture, 0);
    }

    void RenderLayer::destroy(void) {
        parent_renderer->remove_render_layer(this);
        delete this;
    }

    void RenderLayer::render_to_parent(void) {
        parent_renderer->activate_gl_context();
        root_item->render();
    }

}
