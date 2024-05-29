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

#include "argus/lowlevel/math.hpp"

#include "argus/render/common/scene.hpp"
#include "argus/render/common/viewport.hpp"

#include <string>
#include <vector>

#include <cstdint>

namespace argus {
    // forward declarations
    class Camera2D;

    struct pimpl_AttachedViewport;

    struct AttachedViewport {
      protected:
        AttachedViewport(SceneType type);

        AttachedViewport(const AttachedViewport &) = delete;

        AttachedViewport(AttachedViewport &&) = delete;

        virtual pimpl_AttachedViewport *get_pimpl(void) const = 0;

      public:
        SceneType m_type;

        virtual ~AttachedViewport(void) = 0;

        Viewport get_viewport(void) const;

        uint32_t get_z_index(void) const;

        std::vector<std::string> get_postprocessing_shaders(void) const;

        void add_postprocessing_shader(const std::string &shader_uid);

        void remove_postprocessing_shader(const std::string &shader_uid);
    };
}
