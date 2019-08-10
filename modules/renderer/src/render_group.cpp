#include "internal/util.hpp"

#include "argus/renderer.hpp"

namespace argus {

    RenderGroup::RenderGroup(RenderLayer &parent):
            parent(parent),
            renderable_factory(*new RenderableFactory(*this)) {
        //TODO
    }

    void RenderGroup::destroy(void) {
        remove_from_vector(parent.children, this);

        delete this;
    }

    void RenderGroup::render(void) const {
        for (Renderable *const child : this->children) {
            child->render();
        }
    }

    RenderableFactory &RenderGroup::get_renderable_factory(void) {
        return renderable_factory;
    }

}
