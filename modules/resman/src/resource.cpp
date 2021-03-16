/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module resman
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"
#include "internal/resman/pimpl/resource.hpp"

#include <atomic>
#include <string>
#include <utility>
#include <vector>

namespace argus {
    Resource::Resource(ResourceManager &manager, const ResourcePrototype prototype, void *const data_ptr,
            std::vector<std::string> &dependencies):
            prototype(prototype),
            pimpl(new pimpl_Resource(manager, data_ptr, dependencies)) {
    }

    Resource::Resource(Resource &&rhs):
            prototype(std::move(rhs.prototype)),
            pimpl(new pimpl_Resource(rhs.pimpl->manager, rhs.pimpl->data_ptr, rhs.pimpl->dependencies,
                    rhs.pimpl->ref_count.load())) {
    }

    Resource::~Resource(void) {
        delete pimpl;
    }

    void Resource::release(void) {
        if (--pimpl->ref_count == 0) {
            pimpl->manager.unload_resource(prototype.uid);
        }
    }

    void *Resource::get_data_raw_ptr(void) {
        return pimpl->data_ptr;
    }

}
