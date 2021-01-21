/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
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

#include <exception>
#include <initializer_list>
#include <istream>
#include <map>
#include <string>
#include <vector>

#include <cstddef>

namespace argus {

    void ResourceLoader::load_dependencies(ResourceManager &manager, std::initializer_list<std::string> dependencies) {
        std::vector<Resource*> acquired;

        bool failed = false;
        auto it = dependencies.begin();
        std::exception thrown_exception;
        for (it; it < dependencies.end(); it++) {
            try {
                Resource &res = manager.get_resource(*it);
                acquired.insert(acquired.end(), &res);
            } catch (std::exception &ex) {
                failed = true;
                thrown_exception = ex;
                break;
            }
        }

        if (failed) {
            for (Resource *res : acquired) {
                res->release();
            }

            throw thrown_exception;
        }

        pimpl->last_dependencies = dependencies;
    }

    ResourceLoader::ResourceLoader(std::string type_id,
            std::initializer_list<std::string> extensions):
            pimpl(new pimpl_ResourceLoader(type_id, extensions)) {
    }

    ResourceLoader::~ResourceLoader(void) {
        delete pimpl;
    }

    void *const ResourceLoader::load(std::istream &stream, const size_t size) const {
        return nullptr;
    }

    void ResourceLoader::unload(void *const data_ptr) const {
        // This function is intentionally left blank.
    }

}
