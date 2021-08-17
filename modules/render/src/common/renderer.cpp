/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/time.hpp"

// module core
#include "internal/core/core_util.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"

// module render
#include "argus/render/common/renderer.hpp"
#include "argus/render/common/render_layer.hpp"
#include "argus/render/common/render_layer_type.hpp"
#include "argus/render/common/transform.hpp"
#include "argus/render/2d/render_layer_2d.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/renderer.hpp"
#include "internal/render/renderer_impl.hpp"
#include "internal/render/pimpl/common/renderer.hpp"
#include "internal/render/pimpl/common/render_layer.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace argus {
    // forward declarations
    struct ArgusEvent;

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
        g_renderer_impl->deinit(*this);

        for (auto *layer : pimpl->render_layers) {
            delete layer;
        }

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

    RenderLayer &Renderer::create_layer(RenderLayerType type, const int index) {
        RenderLayer *layer;
        if (type == RenderLayerType::Render2D) {
            layer = new RenderLayer2D(*this, Transform2D{}, index);
        } else {
            throw std::invalid_argument("Unsupported layer type");
        }

        pimpl->render_layers.push_back(layer);

        std::sort(pimpl->render_layers.begin(), pimpl->render_layers.end(),
                  [](RenderLayer *a, RenderLayer *b) { return a->get_pimpl()->index < b->get_pimpl()->index; });

        return *layer;
    }

    void Renderer::remove_render_layer(RenderLayer &layer) {
        if (&layer.get_parent_renderer() != this) {
            throw std::invalid_argument("Supplied RenderLayer does not belong to the Renderer");
        }

        remove_from_vector(pimpl->render_layers, &layer);
    }

    void renderer_window_event_callback(const ArgusEvent &event, void *user_data) {
        UNUSED(user_data);
        const WindowEvent &window_event = static_cast<const WindowEvent&>(event);

        if (window_event.subtype != WindowEventType::Create
                && window_event.subtype != WindowEventType::Update
                && window_event.subtype != WindowEventType::RequestClose) {
            return;
        }

        Window &window = window_event.window;

        auto it = g_renderer_map.find(&window);

        if (it == g_renderer_map.cend()) {
            return;
        }

        Renderer &renderer = *it->second;

        switch (window_event.subtype) {
            case WindowEventType::Create: {
                renderer.init();
                break;
            }
            case WindowEventType::Update: {
                if (window.is_ready()) {
                    renderer.render(window_event.delta);
                }
                break;
            }
            case WindowEventType::RequestClose: {
                delete &renderer;
                break;
            }
            default: {
                break;
            }
        }
    }
}
