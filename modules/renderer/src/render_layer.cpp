/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/logging.hpp"

// module core
#include "internal/core_util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

namespace argus {

    using namespace glext;

    extern Shader g_layer_transform_shader;

    static std::vector<const Shader*> _generate_initial_layer_shaders(void) {
        std::vector<const Shader*> shaders;
        shaders.insert(shaders.cbegin(), &g_layer_transform_shader);
        return shaders;
    }

    RenderLayer::RenderLayer(Renderer &parent, const int priority):
            parent_renderer(parent),
            priority(priority),
            shaders(_generate_initial_layer_shaders()),
            def_group(create_render_group(0)),
            transform(),
            children({}),
            dirty_shaders(false) {
    }

    void RenderLayer::destroy(void) {
        parent_renderer.remove_render_layer(*this);
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

    RenderableFactory &RenderLayer::get_renderable_factory(void) {
        return def_group.get_renderable_factory();
    }

    RenderGroup &RenderLayer::create_render_group(const int priority) {
        RenderGroup *group = new RenderGroup(*this);
        children.insert(children.cbegin(), group);
        return *group;
    }

    RenderGroup &RenderLayer::get_default_group(void) {
        return def_group;
    }

    void RenderLayer::add_shader(const Shader &shader) {
        shaders.insert(shaders.cbegin(), &shader);
    }

    void RenderLayer::remove_shader(const Shader &shader) {
        remove_from_vector(shaders, &shader);
    }

    void RenderLayer::render(void) {
        for (RenderGroup *group : children) {
            group->draw();
        }

        if (dirty_shaders) {
            dirty_shaders = false;
        }

        if (transform.is_dirty()) {
            transform.clean();
        }
    }

}
