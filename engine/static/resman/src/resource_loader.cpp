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

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp"
#include "internal/resman/pimpl/resource_loader.hpp"

#include <initializer_list>
#include <map>
#include <string>
#include <vector>

namespace argus {
    Result<void *, ResourceError> ResourceLoader::make_ok_result(void *ptr) {
        return ok<void *, ResourceError>(ptr);
    }

    Result<void *, ResourceError> ResourceLoader::make_err_result(ResourceErrorReason reason,
            const ResourcePrototype &proto, std::string msg) {
        return err<void *, ResourceError>(ResourceError { reason, proto.uid, std::move(msg) });
    }

    Result<std::map<std::string, const Resource *>, ResourceError> ResourceLoader::load_dependencies(
            ResourceManager &manager, const std::vector<std::string> &dependencies) const {
        std::map<std::string, const Resource *> acquired;

        bool failed = false;
        ResourceError res_err;
        for (auto it = dependencies.begin(); it < dependencies.end(); it++) {
            auto res = manager.get_resource(*it);
            if (res.is_ok()) {
                acquired.insert({ res.unwrap().uid, &res.unwrap() });
            } else {
                failed = true;
                res_err = res.unwrap_err();
                break;
            }
        }

        if (failed) {
            for (const auto &[_, res] : acquired) {
                res->release();
            }

            return err<decltype(acquired), ResourceError>(res_err);
        }

        m_pimpl->last_dependencies = dependencies;

        return ok<decltype(acquired), ResourceError>(acquired);
    }

    ResourceLoader::ResourceLoader(std::initializer_list<std::string> media_types):
        m_pimpl(new pimpl_ResourceLoader(media_types)) {
    }

    ResourceLoader::~ResourceLoader(void) {
        delete m_pimpl;
    }
}
