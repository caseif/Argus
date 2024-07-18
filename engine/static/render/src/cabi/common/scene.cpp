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

#include "argus/render/cabi/common/scene.h"

#include "argus/render/common/scene.hpp"

using argus::Scene;

static inline const Scene &_as_ref(argus_scene_const_t scene) {
    return *reinterpret_cast<const Scene *>(scene);
}

extern "C" {

argus_scene_t argus_scene_find(const char *id) {
    auto res = Scene::find(id);
    if (res) {
        return &res.value().get();
    } else {
        return nullptr;
    }
}

SceneType argus_scene_get_type(argus_scene_const_t scene) {
    return SceneType(_as_ref(scene).type);
}

}
