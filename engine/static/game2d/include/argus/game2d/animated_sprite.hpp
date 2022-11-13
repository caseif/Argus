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

#pragma once

#include "argus/lowlevel/math.hpp"

#include <string>
#include <vector>

namespace argus {
    // forward declarations
    struct pimpl_AnimatedSprite;

    class AnimatedSprite {
       public:
        pimpl_AnimatedSprite *pimpl;

        AnimatedSprite(const Vector2f &base_size, float speed);

        AnimatedSprite(const AnimatedSprite&) = delete;

        AnimatedSprite(AnimatedSprite&&);

        ~AnimatedSprite(void);

        const Vector2f &get_base_size(void) const;

        float get_animation_speed(void) const;

        float set_animation_speed(float speed) const;

        std::vector<std::string> get_available_animations(void) const;

        const std::string &get_current_animation(void) const;

        const std::string &set_current_animation(const std::string &animation_id);

        bool does_current_animation_loop(void) const;

        const Padding &get_current_animation_padding(void) const;

        void pause_animation(void);

        void resume_animation(void);

        void reset_animation(void);
    };
}
