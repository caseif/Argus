#include "argus/renderer.hpp"

namespace argus {

    RenderableFactory::RenderableFactory(RenderGroup &parent):
            parent(parent) {
    }

    RenderableTriangle &RenderableFactory::create_triangle(Vertex const &corner_1, Vertex const &corner_2, Vertex const &corner_3) const {
        return *new RenderableTriangle(parent, corner_1, corner_2, corner_3);
    }

    RenderableSquare &RenderableFactory::create_square(Vertex const &corner_1, Vertex const &corner_2,
            Vertex const &corner_3, Vertex const &corner_4) const {
        return *new RenderableSquare(parent, corner_1, corner_2, corner_3, corner_4);
    }

}
