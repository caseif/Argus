#include "argus/renderer.hpp"

#include "vmmlib/vector.hpp"

namespace argus {

    using vmml::vec2f;

    RenderTriangle::RenderTriangle(RenderLayer &parent_layer, RenderItem *const parent_item, vec2f corner_1, vec2f corner_2, vec2f corner_3):
            RenderItem(parent_layer, parent_item) {
        this->corner_1 = corner_1;
        this->corner_2 = corner_2;
        this->corner_3 = corner_3;
    }

    void RenderTriangle::render(void) const {
        render_children();
        //TODO
    }

}
