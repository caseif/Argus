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

#include "argus/core/cabi/event.h"
#include "argus/core/event.hpp"

extern "C" {

Index argus_register_event_handler(const char *type_id, argus_event_handler_t handler,
        TargetThread target_thread, void *data, Ordering ordering) {
    return argus::register_event_handler_with_type(type_id, [&handler](const argus::ArgusEvent &event, void *data) {
        handler(reinterpret_cast<const void *>(&event), data);
    }, argus::TargetThread(target_thread), data, argus::Ordering(ordering));
}

void argus_unregister_event_handler(Index index) {
    argus::unregister_event_handler(index);
}

void argus_dispatch_event(argus_event_t event) {
    argus::_dispatch_event_ptr(*reinterpret_cast<argus::ArgusEvent *>(event));
}

}
