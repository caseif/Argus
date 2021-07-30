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
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp"
#include "internal/resman/pimpl/resource_loader.hpp"

#include <exception> // IWYU pragma: keep
#include <initializer_list>
#include <istream>
#include <map>
#include <string>
#include <vector>

#include <cstddef>

namespace argus {

    std::map<std::string, const Resource*> ResourceLoader::load_dependencies(ResourceManager &manager,
            const std::vector<std::string> &dependencies) const {
        std::map<std::string, const Resource*> acquired;

        bool failed = false;
        std::exception thrown_exception;
        for (auto it = dependencies.begin(); it < dependencies.end(); it++) {
            try {
                auto &res = manager.get_resource(*it);
                acquired.insert({ res.uid, &res });
            } catch (std::exception &ex) {
                failed = true;
                thrown_exception = ex;
                break;
            }
        }

        if (failed) {
            for (auto it : acquired) {
                it.second->release();
            }

            throw thrown_exception;
        }

        pimpl->last_dependencies = dependencies;

        return acquired;
    }
    
    ResourceLoader::ResourceLoader(std::initializer_list<std::string> media_types):
            pimpl(new pimpl_ResourceLoader(media_types)) {
    }

    ResourceLoader::~ResourceLoader(void) {
        delete pimpl;
    }

    void *const ResourceLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, const size_t size) const {
        return nullptr;
    }

    void ResourceLoader::unload(void *const data_ptr) const {
        // This function is intentionally left blank.
    }

}
