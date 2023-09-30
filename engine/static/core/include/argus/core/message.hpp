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

#include "argus/lowlevel/message.hpp"

#include <typeindex>

namespace argus {
    typedef std::function<void(const Message &)> GenericMessagePerformer;

    template <typename T>
    using MessagePerformer = typename std::function<void(const T &)>;

    void register_message_performer(std::type_index type, GenericMessagePerformer performer);

    template <typename T>
    std::enable_if_t<std::is_base_of_v<Message, T>, void> register_message_performer(
            const MessagePerformer<T> &performer) {
        register_message_performer(typeid(T), [performer](const Message &message) {
            performer(reinterpret_cast<const T &>(message));
        });
    }
}
