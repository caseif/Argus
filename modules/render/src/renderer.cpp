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
#include "argus/core.hpp"
#include "internal/core/core_util.hpp"

// module render
#include "argus/render/renderer.hpp"
#include "argus/render/render_layer.hpp"
#include "argus/render/transform.hpp"
#include "argus/render/window.hpp"
#include "internal/render/gl/renderer_gl.hpp"
#include "internal/render/pimpl/renderer.hpp"
#include "internal/render/pimpl/render_layer.hpp"
#include "internal/render/pimpl/window.hpp"

#include <algorithm>
#include <vector>

namespace argus {
    Renderer::Renderer(Window &window, RenderBackend backend):
            pimpl(new pimpl_Renderer(window)) {
        RendererImpl *impl;

        switch (backend) {
            case RenderBackend::OPENGL:
                impl = new GLRenderer(*this);
                break;
            case RenderBackend::VULKAN:
                _ARGUS_FATAL("Vulkan is not yet supported\n");
            default:
                _ARGUS_FATAL("Unrecognized render backend index %d\n", static_cast<int>(backend));
        }

        pimpl->impl = impl;
    }

    Renderer::~Renderer() {
        pimpl->impl->~RendererImpl();
        delete pimpl;
    }

    void Renderer::init_context_hints(void) {
        pimpl->impl->init_context_hints();
    }

    void Renderer::init(void) {
        pimpl->impl->init();
    }

    void Renderer::render(const TimeDelta delta) {
        pimpl->impl->render(delta);
    }

    RenderLayer &Renderer::create_render_layer(const int index) {
        auto layer = new RenderLayer(*this, Transform{}, index);

        pimpl->render_layers.push_back(layer);

        std::sort(pimpl->render_layers.begin(), pimpl->render_layers.end(),
                  [](RenderLayer *a, RenderLayer *b) { return a->pimpl->index < b->pimpl->index; });

        return *layer;
    }

    void Renderer::remove_render_layer(RenderLayer &layer) {
        if (&layer.pimpl->parent_renderer != this) {
            throw std::invalid_argument("Supplied RenderLayer does not belong to the Renderer");
        }

        remove_from_vector(pimpl->render_layers, &layer);
    }
}
