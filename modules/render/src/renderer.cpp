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
#include "internal/core/config.hpp"
#include "internal/core/core_util.hpp"
#include "internal/core/defines.hpp"
#include "internal/core/dyn_invoke.hpp"

// module render
#include "argus/render/renderer.hpp"
#include "argus/render/render_layer.hpp"
#include "argus/render/transform.hpp"
#include "argus/render/window.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/pimpl/renderer.hpp"
#include "internal/render/pimpl/render_layer.hpp"
#include "internal/render/pimpl/window.hpp"

#include <algorithm>
#include <vector>

namespace argus {
    //TODO: figure out the appropriate backend during module init
    static RendererImpl *_create_backend_impl(Renderer &parent) {
        auto backends = get_engine_config().render_backends;

        for (auto backend : backends) {
            switch (backend) {
                case RenderBackend::OPENGL: {
                    auto impl = call_module_fn<RendererImpl*>(std::string(FN_CREATE_OPENGL_BACKEND), parent);
                    _ARGUS_INFO("Selecting OpenGL as graphics backend\n");
                    return impl;
                }
                case RenderBackend::OPENGLES:
                    _ARGUS_INFO("Graphics backend OpenGL ES is not yet supported\n");
                    break;
                case RenderBackend::VULKAN:
                    _ARGUS_INFO("Graphics backend Vulkan is not yet supported\n");
                    break;
                default:
                    _ARGUS_WARN("Skipping unrecognized graphics backend index %d\n", static_cast<int>(backend));
                    break;
            }
            _ARGUS_INFO("Current graphics backend cannot be selected, continuing to next\n");
        }

        _ARGUS_WARN("Failed to select graphics backend from preference list, defaulting to OpenGL\n");
        return call_module_fn<RendererImpl*>(std::string(FN_CREATE_OPENGL_BACKEND), parent);
        return nullptr;
    }

    Renderer::Renderer(Window &window):
            pimpl(new pimpl_Renderer(window, _create_backend_impl(*this))) {
    }

    Renderer::~Renderer() {
        delete pimpl->impl;
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
