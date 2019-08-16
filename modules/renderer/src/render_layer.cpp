// module core
#include "internal/util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

namespace argus {

    using glext::glGenFramebuffers;
    using glext::glFramebufferTexture;

    extern Shader g_layer_transform_shader;

    static std::vector<const Shader*> _generate_initial_layer_shaders(void) {
        std::vector<const Shader*> shaders;
        shaders.insert(shaders.cbegin(), &g_layer_transform_shader);
        return shaders;
    }

    RenderLayer::RenderLayer(Renderer *const parent):
            shaders(_generate_initial_layer_shaders()),
            root_group(RenderGroup(*this)),
            dirty_transform(false),
            dirty_shaders(false) {
        this->parent_renderer = parent;

        children.insert(children.cbegin(), &root_group);
    }

    void RenderLayer::destroy(void) {
        parent_renderer->remove_render_layer(*this);
        delete this;
    }

    void RenderLayer::remove_group(RenderGroup &group) {
        _ARGUS_ASSERT(&group.parent == this, "remove_group() passed group with wrong parent");

        remove_from_vector(children, &group);

        delete this;
    }

    Transform &RenderLayer::get_transform(void) {
        return transform;
    }

    RenderGroup &RenderLayer::create_render_group(const int priority) {
        RenderGroup *group = new RenderGroup(*this);
        children.insert(children.cbegin(), group);
        return *group;
    }

    void RenderLayer::add_shader(Shader const &shader) {
        shaders.insert(shaders.cbegin(), &shader);
    }

    void RenderLayer::remove_shader(Shader const &shader) {
        remove_from_vector(shaders, &shader);
    }

    void RenderLayer::render(void) {
        parent_renderer->activate_gl_context();

        for (RenderGroup *group : children) {
            if (group->dirty_children) {
                group->update_buffer();
            }
        }

        transform.clean();
    }

}
