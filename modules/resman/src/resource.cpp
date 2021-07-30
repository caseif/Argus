/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/memory.hpp"
#include "internal/lowlevel/logging.hpp"

// module resman
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp"
#include "internal/resman/pimpl/resource.hpp"
#include "internal/resman/pimpl/resource_manager.hpp"

#include "arp/arp.h"

#include <atomic>
#include <string>
#include <utility>
#include <vector>

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_Resource));
    
    ResourcePrototype ResourcePrototype::from_arp_meta(std::string uid, const arp_resource_meta_t &meta) {
        return ResourcePrototype(uid, meta.media_type, "");
    }

    Resource::Resource(ResourceManager &manager, const ResourceLoader &loader, const ResourcePrototype prototype,
            void *const data_ptr, std::vector<std::string> &dependencies):
        prototype(prototype),
        pimpl(&g_pimpl_pool.construct<pimpl_Resource>(manager, loader, data_ptr, dependencies)) {
    }

    Resource::Resource(Resource &&rhs):
            prototype(std::move(rhs.prototype)),
            pimpl(rhs.pimpl) {
    }

    Resource::~Resource(void) {
        g_pimpl_pool.free(pimpl);
    }

    void Resource::release(void) const {
        unsigned int new_ref_count = pimpl->ref_count.fetch_sub(1) - 1;
        _ARGUS_DEBUG("Releasing handle on resource %s (new refcount is %d)\n",
                this->prototype.uid.c_str(), new_ref_count);
        if (new_ref_count == 0) {
            pimpl->manager.unload_resource(prototype.uid);
        }
    }

    const void *Resource::get_data_raw_ptr(void) const {
        return pimpl->data_ptr;
    }
}
