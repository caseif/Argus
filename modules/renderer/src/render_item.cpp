#include "internal/util.hpp"

#include "argus/renderer.hpp"

namespace argus {

    RenderItem::RenderItem(RenderLayer &layer, RenderItem *const parent):
                    parent(parent),
                    layer(layer) {
    }

    void RenderItem::render_children(void) const {
        for (RenderItem *const child : this->children) {
            child->render();
        }
    }

    void RenderItem::render(void) const {
        //TODO
    }

    void RenderItem::destroy(void) {
        if (parent != nullptr) {
            remove_from_vector(parent->children, this);
        }
    }

}
