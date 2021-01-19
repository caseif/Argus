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

// module wm
#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"
#include "internal/wm/pimpl/window.hpp"

// module render
#include "argus/render/renderer.hpp"
#include "argus/render/render_layer.hpp"
#include "argus/render/transform.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/pimpl/renderer.hpp"
#include "internal/render/pimpl/render_layer.hpp"

#include <algorithm>
#include <map>
#include <vector>

namespace argus {
    Renderer &Renderer::of_window(Window &window) {
        auto it = g_renderer_map.find(&window);
        if (it == g_renderer_map.end()) {
            throw std::runtime_error("No Renderer attached to requested Window");
        }
        return *it->second;
    }

    Renderer::Renderer(Window &window):
            pimpl(new pimpl_Renderer(window)) {
    }

    Renderer::~Renderer() {
        g_renderer_map.erase(&pimpl->window);
        delete pimpl;
    }

    Window &Renderer::get_window(void) const {
        return pimpl->window;
    }

    void Renderer::init(void) {
        get_renderer_impl().init(*this);
    }

    void Renderer::render(const TimeDelta delta) {
        get_renderer_impl().render(*this, delta);
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

    void renderer_window_event_callback(const ArgusEvent &event, void *user_data) {
        const WindowEvent &window_event = static_cast<const WindowEvent&>(event);

        if (window_event.subtype != WindowEventType::CREATE
                && window_event.subtype != WindowEventType::UPDATE
                && window_event.subtype != WindowEventType::REQUEST_CLOSE) {
            return;
        }

        Window &window = window_event.window;

        auto it = g_renderer_map.find(&window);

        if (it == g_renderer_map.cend()) {
            return;
        }

        Renderer &renderer = *it->second;

        switch (window_event.subtype) {
            case WindowEventType::CREATE: {
                renderer.init();
                break;
            }
            case WindowEventType::UPDATE: {
                if (window.is_ready()) {
                    renderer.render(window_event.delta);
                }
                break;
            }
            case WindowEventType::REQUEST_CLOSE: {
                delete &renderer;
                break;
            }
        }
    }
}
