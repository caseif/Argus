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

// module core
#include "internal/core/core_util.hpp"

// module renderer
#include "argus/renderer/render_group.hpp"
#include "argus/renderer/render_layer.hpp"
#include "argus/renderer/renderable_factory.hpp"
#include "argus/renderer/renderer.hpp"
#include "argus/renderer/shader.hpp"
#include "argus/renderer/transform.hpp"
#include "internal/renderer/glext.hpp"
#include "internal/renderer/pimpl/render_group.hpp"
#include "internal/renderer/pimpl/render_layer.hpp"

#include <vector>

namespace argus {

    using namespace glext;

    extern Shader g_layer_transform_shader;

    static std::vector<const Shader*> _generate_initial_layer_shaders(void) {
        std::vector<const Shader*> shaders;
        shaders.insert(shaders.cbegin(), &g_layer_transform_shader);
        return shaders;
    }

    RenderLayer::RenderLayer(Renderer &parent, const int priority):
            pimpl(new pimpl_RenderLayer(parent, priority)) {
        pimpl->shaders = _generate_initial_layer_shaders();
        pimpl->def_group = &create_render_group(0);
        pimpl->children = {pimpl->def_group};
        pimpl->dirty_shaders = false;
    }

    void RenderLayer::destroy(void) {
        pimpl->parent_renderer.remove_render_layer(*this);
        delete pimpl;
        delete this;
    }

    void RenderLayer::remove_group(RenderGroup &group) {
        _ARGUS_ASSERT(&group.pimpl->parent == this, "remove_group() passed group with wrong parent");

        remove_from_vector(pimpl->children, &group);

        delete this;
    }

    Transform &RenderLayer::get_transform(void) {
        return pimpl->transform;
    }

    RenderableFactory &RenderLayer::get_renderable_factory(void) {
        return pimpl->def_group->get_renderable_factory();
    }

    RenderGroup &RenderLayer::create_render_group(const int priority) {
        RenderGroup *group = new RenderGroup(*this);
        pimpl->children.insert(pimpl->children.cbegin(), group);
        return *group;
    }

    RenderGroup &RenderLayer::get_default_group(void) {
        return *pimpl->def_group;
    }

    void RenderLayer::add_shader(const Shader &shader) {
        pimpl->shaders.insert(pimpl->shaders.cbegin(), &shader);
    }

    void RenderLayer::remove_shader(const Shader &shader) {
        remove_from_vector(pimpl->shaders, &shader);
    }

    void RenderLayer::render(void) {
        for (RenderGroup *group : pimpl->children) {
            group->draw();
        }

        if (pimpl->dirty_shaders) {
            pimpl->dirty_shaders = false;
        }

        if (pimpl->transform.is_dirty()) {
            pimpl->transform.clean();
        }
    }

}
