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

#pragma once

#include <typeindex>
#include <vector>

namespace argus {
    class Entity;

    class EntityBuilder {
        friend class Entity;

        private:
            std::vector<std::type_index> types;

            EntityBuilder(void);

            ~EntityBuilder(void);

        public:
            template <typename T>
            EntityBuilder &with(void) {
                return with(std::type_index(typeid(T)));
            }

            EntityBuilder &with(std::type_index type);

            Entity &build(void);
    };
}
