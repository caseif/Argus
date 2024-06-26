/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/memory.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp"
#include "internal/resman/pimpl/resource.hpp"

#include <string>
#include <utility>
#include <vector>

namespace argus {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_Resource));

    Resource::Resource(ResourceManager &manager, const ResourceLoader &loader, ResourcePrototype prototype,
            void *const data_ptr, const std::vector<std::string> &dependencies):
        m_pimpl(&g_pimpl_pool.construct<pimpl_Resource>(manager, loader, data_ptr, dependencies)),
        prototype(std::move(prototype)) {
    }

    Resource::Resource(Resource &&rhs) noexcept:
        m_pimpl(rhs.m_pimpl),
        prototype(rhs.prototype) {
    }

    Resource::~Resource(void) {
        g_pimpl_pool.destroy(m_pimpl);
    }

    void Resource::release(void) const {
        unsigned int new_ref_count = m_pimpl->ref_count.fetch_sub(1) - 1;
        Logger::default_logger().debug("Releasing handle on resource %s (new refcount is %d)",
                this->prototype.uid.c_str(), new_ref_count);
        if (new_ref_count == 0) {
            auto res = m_pimpl->manager.unload_resource(prototype.uid);
            if (res.is_err()) {
                Logger::default_logger().warn("Failed to unload resource %s: %s (%d)", prototype.uid.c_str(),
                        res.unwrap_err().info.c_str(), res.unwrap_err().reason);
            }
        }
    }

    const void *Resource::get_data_ptr(void) const {
        return m_pimpl->data_ptr;
    }
}
