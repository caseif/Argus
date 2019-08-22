// module lowlevel
#include "internal/logging.hpp"

// module core
#include "internal/util.hpp"

// module renderer
#include "argus/renderer.hpp"

namespace argus {

    Renderable::Renderable(RenderGroup &group):
            parent(group),
            transform(Transform()) {
        parent.add_renderable(*this);
    }

    void Renderable::remove(void) {
        parent.remove_renderable(*this);

        delete this;
    }

    Transform const &Renderable::get_transform(void) const {
        return transform;
    }

}
