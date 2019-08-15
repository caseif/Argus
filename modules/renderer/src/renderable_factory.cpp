#include "argus/renderer.hpp"

namespace argus {

    using vmml::vec2d;

    RenderableFactory::RenderableFactory(RenderGroup &parent):
            parent(parent) {
    }

    RenderableTriangle &RenderableFactory::create_triangle(vec2d const &corner_1, vec2d const &corner_2, vec2d const &corner_3) const {
        RenderableTriangle &triangle = *new RenderableTriangle(parent, corner_1, corner_2, corner_3);
        parent.add_renderable(triangle);
        return triangle;
    }

}
