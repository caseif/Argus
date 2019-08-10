#include "internal/util.hpp"

#include "argus/renderer.hpp"

namespace argus {

    Renderable::Renderable(RenderGroup &group):
                    parent(group) {
    }

    void Renderable::destroy(void) {
        remove_from_vector(parent.children, this);

        delete this;
    }

}
