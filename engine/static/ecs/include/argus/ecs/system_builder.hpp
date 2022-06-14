/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#pragma once

#include "argus/ecs/system.hpp"

#include <functional>
#include <string>
#include <typeindex>
#include <vector>

namespace argus {
    class Entity;
    class System;

    class SystemBuilder {
        friend class System;

        private:
            std::string name;
            std::vector<std::type_index> types;
            EntityCallback callback;

            SystemBuilder(void) = default;

        public:
            SystemBuilder &with_name(std::string name);

            template <typename T>
            SystemBuilder &targets(void) {
                return targets(std::type_index(typeid(T)));
            }

            SystemBuilder &targets(std::type_index type);

            SystemBuilder &with_callback(EntityCallback callback);

            System &build(void);
    };
}
