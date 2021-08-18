/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module render
#include "argus/render/common/scene.hpp"
#include "argus/render/common/transform.hpp"
#include "internal/render/pimpl/common/scene.hpp"

namespace argus {
    class Renderer;

    Scene::Scene(SceneType type):
        type(type) {
    }

    Scene::~Scene(void) {
    }

    const Renderer &Scene::get_parent_renderer(void) const {
        return get_pimpl()->parent_renderer;
    }

    Transform2D &Scene::get_transform(void) const {
        return get_pimpl()->transform;
    }

    void Scene::set_transform(Transform2D &transform) {
        get_pimpl()->transform = transform;
    }
}
