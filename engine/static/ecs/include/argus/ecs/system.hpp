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

#pragma once

#include "argus/lowlevel/time.hpp"

#include <functional>
#include <string>
#include <typeindex>
#include <vector>

namespace argus {
    // forward declarations
    class Entity;

    class SystemBuilder;

    struct pimpl_System;

    typedef std::function<void(const Entity &, TimeDelta delta)> EntityCallback;

    class System {
      private:
        System(std::string name, std::vector<std::type_index> component_types, EntityCallback callback);

        System(System &) = delete;

        System(System &&) = delete;

      public:
        pimpl_System *m_pimpl;

        static SystemBuilder builder(void);

        static System &create(std::string name, std::vector<std::type_index> component_types,
                EntityCallback callback);

        ~System(void);

        const std::string get_name(void);

        bool is_active(void);

        void set_active(bool active);
    };
}
