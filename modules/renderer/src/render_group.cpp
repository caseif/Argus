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

    void RenderGroup::update_buffer(void) {
        //TODO

        for (Renderable *const child : this->children) {
            child->render();
        }

        dirty = false;
    }

    void RenderGroup::add_renderable(Renderable &renderable) {
        children.insert(children.cbegin(), &renderable);
        dirty = true;
    }

    void RenderGroup::remove_renderable(Renderable &renderable) {
        remove_from_vector(children, &renderable);
        dirty = true;
    }

    RenderableFactory &RenderGroup::get_renderable_factory(void) const {
        return renderable_factory;
    }

}
