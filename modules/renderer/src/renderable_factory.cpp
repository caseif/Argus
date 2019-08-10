#include "argus/renderer.hpp"

namespace argus {

    using vmml::vec2d;

    RenderableFactory::RenderableFactory(RenderGroup &parent):
            parent(parent) {
    }

    RenderableTriangle &RenderableFactory::create_triangle(vec2d &corner_1, vec2d &corner_2, vec2d &corner_3) {
        return *new RenderableTriangle(parent, corner_1, corner_2, corner_3);
    }

}
