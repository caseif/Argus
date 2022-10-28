/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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
#include "internal/render/pimpl/common/scene.hpp"

#include <algorithm>
#include <functional>
#include <optional>
#include <string>

namespace argus {
    class Canvas;

    std::optional<std::reference_wrapper<Scene>> Scene::find(const std::string &id) {
        //TODO: implement
    }

    Scene::Scene(SceneType type):
        type(type) {
    }

    Scene::~Scene(void) {
    }

    Transform2D Scene::peek_transform(void) const {
        return get_pimpl()->transform.peek();
    }

    ValueAndDirtyFlag<Transform2D> Scene::get_transform(void) {
        return get_pimpl()->transform.read();
    }

    void Scene::set_transform(const Transform2D &transform) {
        get_pimpl()->transform = transform;
    }

    std::vector<std::string> Scene::get_postprocessing_shaders(void) const {
        return get_pimpl()->postfx_shader_uids;
    }

    void Scene::add_postprocessing_shader(const std::string &shader_uid) {
        get_pimpl()->postfx_shader_uids.push_back(shader_uid);
    }

    void Scene::remove_postprocessing_shader(const std::string &shader_uid) {
        auto uids = get_pimpl()->postfx_shader_uids;
        auto it = std::find(uids.crbegin(), uids.crend(), shader_uid);
        if (it != uids.crend()) {
            // some voodoo because it's a reverse iterator
            std::advance(it, 1);
            uids.erase(it.base());
        }
    }
}
