#include "argus/renderer.hpp"

namespace argus {

    RenderItemFactory::RenderItemFactory(RenderLayer &parent):
            parent(parent) {
    }

    RenderTriangle &RenderItemFactory::create_trangle(vec2d &corner_1, vec2d &corner_2, vec2d &corner_3) {
        return *new RenderTriangle(parent, &parent.root_item, corner_1, corner_2, corner_3);
    }

}
