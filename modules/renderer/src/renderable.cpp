#include "internal/util.hpp"

#include "argus/renderer.hpp"

namespace argus {

    Renderable::Renderable(RenderGroup &group):
                    parent(group) {
        parent.add_renderable(*this);
    }

    void Renderable::destroy(void) {
        parent.remove_renderable(*this);

        delete this;
    }

}
