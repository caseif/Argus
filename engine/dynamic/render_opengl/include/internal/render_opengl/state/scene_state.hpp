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

#include "argus/lowlevel/handle.hpp"
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/uuid.hpp"

#include "argus/render/util/linked_program.hpp"
#include "argus/render/util/object_processor.hpp"

#include "internal/render_opengl/types.hpp"

#include <map>
#include <string>

namespace argus {
    // forward declarations
    class Material;

    class RenderObject2D;

    class Scene;

    struct ProcessedRenderObject;
    struct RenderBucket;
    struct RendererState;

    struct BucketKey {
        const std::string material_uid;
        const Vector2f atlas_stride;
        const uint32_t z_index;
        const float light_opacity;
    };

    struct BucketKeyCmp {
        bool operator()(const BucketKey &lhs, const BucketKey &rhs) const {
            return std::tie(lhs.z_index, lhs.light_opacity, lhs.atlas_stride.x, lhs.atlas_stride.y, lhs.material_uid)
                    < std::tie(rhs.z_index, rhs.light_opacity, rhs.atlas_stride.x, rhs.atlas_stride.y,
                            rhs.material_uid);
        }
    };

    struct SceneState {
        RendererState &parent_state;

        Scene &scene;

        BufferInfo ubo {};

        std::map<BucketKey, RenderBucket *, BucketKeyCmp> render_buckets;

        SceneState(RendererState &parent_state, Scene &scene);

        ~SceneState(void);
    };

    struct Scene2DState : public SceneState {
        std::map<Handle, ProcessedRenderObject2DPtr> processed_objs;

        Scene2DState(RendererState &parent_state, Scene &scene);

        Scene2DState(const Scene2DState &rhs) = delete;

        ~Scene2DState(void);
    };
}
