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

#include "argus/resman/cabi/resource_loader.h"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/lowlevel/result.hpp"

#include <istream>
#include <string>
#include <typeindex>
#include <vector>

#include <cstddef>

using argus::ResourceError;
using argus::ResourceLoader;
using argus::ResourceManager;
using argus::ResourcePrototype;

using argus::Result;

class ProxiedResourceLoader : public ResourceLoader {
    size_t m_type_len;
    argus_resource_load_fn_t m_load_fn;
    argus_resource_copy_fn_t m_copy_fn;
    argus_resource_unload_fn_t m_unload_fn;
    void *m_user_data;

  public:
    ProxiedResourceLoader(std::vector<std::string> media_types, size_t type_len,
            argus_resource_load_fn_t load_fn,
            argus_resource_copy_fn_t copy_fn,
            argus_resource_unload_fn_t unload_fn, void *user_data);

    Result<void *, ResourceError> load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) override;

    Result<void *, ResourceError> copy(ResourceManager &manager, const ResourcePrototype &proto,
            const void *src, std::optional<std::type_index> type) override;

    void unload(void *data_ptr) override;

    using ResourceLoader::load_dependencies;
};
