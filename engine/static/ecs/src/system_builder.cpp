/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

#include "argus/ecs/system.hpp"
#include "argus/ecs/system_builder.hpp"

#include <stdexcept>
#include <string>
#include <typeindex>

namespace argus {
    SystemBuilder &SystemBuilder::with_name(std::string name) {
        if (name.empty()) {
            throw std::invalid_argument("System name must be non-empty");
        }

        this->name = name;

        return *this;
    }

    SystemBuilder &SystemBuilder::targets(std::type_index type) {
        this->types.push_back(type);

        return *this;
    }

    SystemBuilder &SystemBuilder::with_callback(EntityCallback callback) {
        if (callback == nullptr) {
            throw std::invalid_argument("System callback must be non-null"); 
        }

        this->callback = callback;

        return *this;
    }

    System &SystemBuilder::build(void) {
        if (this->name.empty()) {
            throw std::runtime_error("Name must be supplied before building system");
        }

        if (this->types.empty()) {
            throw std::runtime_error("At least one component type must be supplied before building system");
        }

        if (this->callback == nullptr) {
            throw std::runtime_error("Callback must be supplied before building system");
        }
        
        return System::create(this->name, this->types, this->callback);
    }
}
