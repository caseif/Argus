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
    template <typename, typename T>
    struct has_message_type_id_accessor : std::false_type {};

    template <typename T>
    struct has_message_type_id_accessor<T, std::void_t<decltype(T::get_message_type_id())>> : std::true_type {};

    template <typename T>
    constexpr bool has_message_type_id_accessor_v = has_message_type_id_accessor<T, void>::value;

    class Message {
      private:
        std::string m_type_id;

      protected:
        Message(std::string type);

        Message(const Message &);

        Message(Message &&) noexcept;

        Message &operator=(const Message &);

        Message &operator=(Message &&);

        ~Message(void);

      public:
        [[nodiscard]] const std::string &get_type_id(void) const;

        template <typename T>
        const T &as(void) const {
            static_assert(has_message_type_id_accessor_v<T>,
                    "Message class must contain static function get_message_type_id");

            assert(T::get_message_type_id() == m_type_id);
            return reinterpret_cast<const T &>(*this);
        }

        template <typename T>
        T &as(void) {
            static_assert(has_message_type_id_accessor_v<T>,
                    "Message class must contain static function get_message_type_id");

            assert(T::get_message_type_id() == m_type_id);
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
