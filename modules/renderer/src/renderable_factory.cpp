#include "argus/renderer.hpp"

namespace argus {

    using vmml::vec2f;

    RenderableFactory::RenderableFactory(RenderGroup &parent):
            parent(parent) {
    }

    RenderableTriangle &RenderableFactory::create_triangle(vec2f const &corner_1, vec2f const &corner_2, vec2f const &corner_3) const {
        return *new RenderableTriangle(parent, corner_1, corner_2, corner_3);
    }

}
