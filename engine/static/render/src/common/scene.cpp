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

#include "argus/render/common/scene.hpp"
#include "argus/render/common/transform.hpp"
#include "internal/render/common/scene.hpp"
#include "internal/render/pimpl/common/scene.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>

namespace argus {
    class Canvas;

    std::map<std::string, Scene *> g_scenes;

    std::optional<std::reference_wrapper<Scene>> Scene::find(const std::string &id) {
        auto it = g_scenes.find(id);
        return it != g_scenes.end() ? std::make_optional(std::reference_wrapper(*it->second)) : std::nullopt;
    }

    Scene::Scene(SceneType type) :
            type(type) {
    }

    Scene::~Scene(void) {
    }
}
