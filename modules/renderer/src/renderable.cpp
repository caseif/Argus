// module lowlevel
#include "internal/logging.hpp"

// module core
#include "internal/core_util.hpp"

// module resman
#include "argus/resource_manager.hpp"

// module renderer
#include "argus/renderer.hpp"

namespace argus {

    Renderable::Renderable(RenderGroup &group):
            parent(group),
            transform(Transform()),
            tex_resource(nullptr) {
        parent.add_renderable(*this);
    }

    Renderable::~Renderable(void) {
        release_texture();
    }

    void Renderable::remove(void) {
        parent.remove_renderable(*this);

        delete this;
    }

    Transform const &Renderable::get_transform(void) const {
        return transform;
    }

    void Renderable::set_texture(std::string const &texture_uid) {
        release_texture();

        if (ResourceManager::get_global_resource_manager().get_resource(texture_uid, &tex_resource) != 0) {
            _ARGUS_WARN("Failed to set texture of Renderable: %s\n", get_error().c_str());
        }
    }

    void Renderable::release_texture(void) {
        if (tex_resource == nullptr) {
            return;
        }

        Resource *res;
        if (ResourceManager::get_global_resource_manager().get_resource(tex_resource->uid, &res) == 0) {
            tex_resource->release();
        } else {
            _ARGUS_WARN("Previous texture %s for Renderable was invalid\n", ((std::string) tex_resource->uid).c_str());
        }
    }

}
