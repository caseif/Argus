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

#include "argus/core/message.hpp"
#include "internal/core/message.hpp"

#include <map>
#include <vector>

namespace argus {
    static std::map<std::string, std::vector<GenericMessagePerformer>> g_performers;

    void register_message_performer(const std::string &type_id, GenericMessagePerformer performer) {
        g_performers[type_id].push_back(std::move(performer));
    }

    void dispatch_message(const Message &message) {
        auto it = g_performers.find(message.get_type_id());
        if (it == g_performers.cend()) {
            return;
        }

        for (const auto &performer : it->second) {
            performer(message);
        }
    }
}
