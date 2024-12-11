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

#pragma once

#include "argus/lowlevel/message.hpp"

namespace argus {
    constexpr const char *MESSAGE_TYPE_OBJECT_DESTROYED = "object_destroyed";

    class ObjectDestroyedMessage {
      public:
        static constexpr const char *get_message_type_id(void) {
            return MESSAGE_TYPE_OBJECT_DESTROYED;
        }

        std::type_index m_type_id;
        void *m_ptr;

        ObjectDestroyedMessage(std::type_index type_id, void *ptr);
    };

    class AutoCleanupable {
      public:
        AutoCleanupable(void);

        AutoCleanupable(const AutoCleanupable &);

        AutoCleanupable(AutoCleanupable &&) noexcept;

        AutoCleanupable &operator=(const AutoCleanupable &);

        AutoCleanupable &operator=(AutoCleanupable &&) noexcept;

        virtual ~AutoCleanupable(void) = 0;
    };
}
