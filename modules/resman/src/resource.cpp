/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/resource_manager.hpp"

namespace argus {

    Resource::Resource(ResourceManager &manager, const ResourcePrototype prototype, void *const data_ptr,
            std::vector<std::string> &dependencies):
            manager(manager),
            prototype(prototype),
            data_ptr(data_ptr),
            dependencies(dependencies),
            ref_count(0) {
    }

    Resource::Resource(Resource &rhs):
            manager(rhs.manager),
            prototype(rhs.prototype),
            data_ptr(rhs.data_ptr),
            ref_count(rhs.ref_count.load()) {
    }

    Resource::Resource(Resource &&rhs):
            manager(rhs.manager),
            prototype(std::move(rhs.prototype)),
            data_ptr(rhs.data_ptr),
            ref_count(rhs.ref_count.load()) {
    }

    void Resource::release(void) {
        if (--ref_count == 0) {
            manager.unload_resource(prototype.uid);
        }
    }

}
