#include "argus/renderer.hpp"

#include "vmmlib/vector.hpp"

namespace argus {

    using vmml::vec2f;

    RenderableTriangle::RenderableTriangle(RenderGroup &parent, vec2f corner_1, vec2f corner_2, vec2f corner_3):
            Renderable(parent) {
        this->corner_1 = corner_1;
        this->corner_2 = corner_2;
        this->corner_3 = corner_3;
    }

    void RenderableTriangle::render(void) const {
        //TODO
    }

}
