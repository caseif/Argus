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

#include "argus/lowlevel/message.hpp"

namespace argus {
    static MessageDispatcher g_dispatcher = nullptr;

    void set_message_dispatcher(MessageDispatcher dispatcher) {
        g_dispatcher = dispatcher;
    }

    void broadcast_message(const std::string &type_id, const void *message) {
        if (g_dispatcher != nullptr) {
            g_dispatcher(type_id.c_str(), &message);
        } else {
            // this is super spammy when running tests
            //Logger::default_logger().warn("Message will not be broadcast (no dispatcher is set)");
        }
    }
}
