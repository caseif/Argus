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

#include "argus/lowlevel/dirtiable.hpp"
#include "argus/lowlevel/handle.hpp"

#include "argus/render/common/scene.hpp"
#include "argus/render/common/transform.hpp"

#include <map>
#include <utility>
#include <vector>

namespace argus {
    struct pimpl_Scene {
        std::string id;
        Dirtiable<Transform2D> transform;

        std::map<Handle, uint16_t> last_rendered_versions;

        std::mutex read_lock;

        pimpl_Scene(std::string id, const Transform2D &transform):
            id(std::move(id)),
            transform(transform) {
        }

        pimpl_Scene(const pimpl_Scene &) = delete;

        pimpl_Scene(pimpl_Scene &&) = delete;
    };
}
