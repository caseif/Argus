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

#include "argus/resman/cabi/resource_event.h"

#include "argus/resman/resource_event.hpp"

using argus::ResourceEvent;

static inline ResourceEvent &_as_ref(argus_resource_event_t ptr) {
    return *reinterpret_cast<ResourceEvent *>(ptr);
}

static inline const ResourceEvent &_as_ref_const(argus_resource_event_const_t ptr) {
    return *reinterpret_cast<const ResourceEvent *>(ptr);
}

#ifdef __cplusplus
extern "C" {
#endif

ResourceEventType argus_resource_event_get_subtype(argus_resource_event_const_t event) {
    return ResourceEventType(_as_ref_const(event).subtype);
}

argus_resource_prototype_t argus_resource_event_get_prototype(argus_resource_event_const_t event) {
    const auto &proto = _as_ref_const(event).prototype;
    argus_resource_prototype_t wrapped_proto {
        proto.uid.c_str(),
        proto.media_type.c_str(),
        reinterpret_cast<const char *>(proto.fs_path.c_str()), // workaround for MSVC
    };
    return wrapped_proto;
}

argus_resource_t argus_resource_event_get_resource(argus_resource_event_t event) {
    return _as_ref(event).resource;
}

#ifdef __cplusplus
}
#endif
