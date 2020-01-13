/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module resman
#include "argus/resource_manager.hpp"

#include <exception>
#include <initializer_list>
#include <istream>
#include <map>
#include <string>
#include <vector>

#include <cstddef>

namespace argus {

    void ResourceLoader::load_dependencies(std::initializer_list<std::string> dependencies) {
        std::vector<Resource*> acquired;

        bool failed = false;
        auto it = dependencies.begin();
        std::exception thrown_exception;
        for (it; it < dependencies.end(); it++) {
            try {
                Resource &res = ResourceManager::get_global_resource_manager().get_resource(*it);
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

        last_dependencies = dependencies;
    }

    ResourceLoader::ResourceLoader(std::string type_id,
            std::initializer_list<std::string> extensions):
            type_id(type_id),
            extensions(extensions) {
        for (std::string ext : extensions) {
            ResourceManager::get_global_resource_manager().extension_registrations.insert({ext, type_id});
        }
    }

    void *const ResourceLoader::load(std::istream &stream, const size_t size) const {
        return nullptr;
    }

    void ResourceLoader::unload(void *const data_ptr) const {
    }

}
