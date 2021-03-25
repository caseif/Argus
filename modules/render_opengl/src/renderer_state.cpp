// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module render
#include "argus/render/common/render_layer.hpp"
#include "argus/render/common/render_layer_type.hpp"

// module render_opengl
#include "internal/render_opengl/globals.hpp"
#include "internal/render_opengl/layer_state.hpp"
#include "internal/render_opengl/renderer_state.hpp"

#include <map>
#include <vector>

namespace argus {
    // forward declarations
    class Renderer;
    class RenderLayer2D;

    RendererState::RendererState(Renderer &renderer):
        renderer(renderer) {
    }

    RendererState::~RendererState(void) {
        for (auto &layer_state : this->all_layer_states) {
            layer_state->~LayerState();
        }
    }
    
    LayerState &RendererState::get_layer_state(RenderLayer &layer, bool create) {
        switch (layer.type) {
            case RenderLayerType::Render2D: {
                auto &layer_2d = reinterpret_cast<const RenderLayer2D&>(layer);
                auto it = this->layer_states_2d.find(&layer_2d);
                if (it != this->layer_states_2d.cend()) {
                    return it->second;
                }

                if (!create) {
                    _ARGUS_FATAL("Failed to get layer state");
                }

                Layer2DState state = Layer2DState(*this, layer);
                auto insert_res = this->layer_states_2d.insert({&layer_2d, state});
                if (!insert_res.second) {
                    _ARGUS_FATAL("Failed to create new layer state");
                }

                return insert_res.first->second;
            }
            case RenderLayerType::Render3D: {
                _ARGUS_FATAL("Unimplemented layer type");
            }
            default: {
                _ARGUS_FATAL("Unrecognized layer type");
            }
        }
    }
}
