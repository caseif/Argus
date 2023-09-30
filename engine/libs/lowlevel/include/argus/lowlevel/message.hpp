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

#include "argus/lowlevel/debug.hpp"

#include <typeindex>

namespace argus {
    class Message {
      private:
        std::type_index m_type;

      protected:
        Message(std::type_index type);

        Message(const Message &);

        Message(Message &&);

        Message &operator=(const Message &);

        Message &operator=(Message &&);

        ~Message(void);

      public:
        std::type_index get_type(void) const;

        template <typename T>
        const T &as(void) const {
            assert(std::type_index(typeid(T)) == m_type);
            return reinterpret_cast<const T &>(*this);
        }

        template <typename T>
        T &as(void) {
            assert(std::type_index(typeid(T)) == m_type);
            return reinterpret_cast<T &>(*this);
        }
    };

    typedef void (*MessageDispatcher)(const Message &);

    void set_message_dispatcher(MessageDispatcher dispatcher);

    void broadcast_message(const Message &message);

    template <typename T>
    std::enable_if_t<std::is_base_of_v<Message, T>, void> broadcast_message(const T &message) {
        broadcast_message(reinterpret_cast<const Message &>(message));
    }
}
