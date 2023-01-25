/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/macros.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp"
#include "internal/resman/pimpl/resource_loader.hpp"

#include <exception>
#include <initializer_list>
#include <istream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cstddef>

namespace argus {
    std::map<std::string, const Resource *> ResourceLoader::load_dependencies(ResourceManager &manager,
            const std::vector<std::string> &dependencies) const {
        std::map<std::string, const Resource *> acquired;

        bool failed = false;
        std::exception *thrown_exception = nullptr;
        for (auto it = dependencies.begin(); it < dependencies.end(); it++) {
            try {
                auto &res = manager.get_resource(*it);
                acquired.insert({res.uid, &res});
            } catch (std::exception &ex) {
                failed = true;
                thrown_exception = &ex;
                break;
            }
        }

        if (failed) {
            for (const auto &res : acquired) {
                res.second->release();
            }

            if (thrown_exception != nullptr) {
                throw *thrown_exception;
            }
        }

        pimpl->last_dependencies = dependencies;

        return acquired;
    }

    ResourceLoader::ResourceLoader(std::initializer_list<std::string> media_types) :
            pimpl(new pimpl_ResourceLoader(media_types)) {
    }

    ResourceLoader::~ResourceLoader(void) {
        delete pimpl;
    }
}
