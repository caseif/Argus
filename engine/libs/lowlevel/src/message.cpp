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

#include "argus/lowlevel/message.hpp"

#include <typeindex>

namespace argus {
    static MessageDispatcher g_dispatcher = nullptr;

    Message::Message(std::type_index type) : m_type(type) {
    }

    Message::Message(const Message &) = default;

    Message::Message(Message &&) = default;

    Message &Message::operator=(const Message &) = default;

    Message &Message::operator=(Message &&) = default;

    Message::~Message(void) = default;

    std::type_index Message::get_type(void) const {
        return m_type;
    }

    void set_message_dispatcher(MessageDispatcher dispatcher) {
        g_dispatcher = dispatcher;
    }

    void broadcast_message(const Message &message) {
        if (g_dispatcher != nullptr) {
            g_dispatcher(message);
        } else {
            // this is super spammy when running tests
            //Logger::default_logger().warn("Message will not be broadcast (no dispatcher is set)");
        }
    }
}