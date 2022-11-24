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

#include "argus/lowlevel/memory.hpp"

#include "argus/game2d/animated_sprite.hpp"
#include "internal/game2d/pimpl/animated_sprite.hpp"
#include "internal/game2d/animated_sprite.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_AnimatedSprite));

    AnimatedSprite::AnimatedSprite(const Vector2f &base_size, float speed):
            pimpl(&g_pimpl_pool.construct<pimpl_AnimatedSprite>(base_size, speed)) {
    }

    AnimatedSprite::AnimatedSprite(AnimatedSprite &&rhs):
            pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    AnimatedSprite::~AnimatedSprite(void) {
        g_pimpl_pool.destroy(pimpl);
    }

    const Vector2f &AnimatedSprite::get_base_size(void) const {
        return pimpl->base_size;
    }

    float AnimatedSprite::get_animation_speed(void) const {
        return pimpl->speed;
    }

    void AnimatedSprite::set_animation_speed(float speed) const {
        pimpl->speed = speed;
    }

    std::vector<std::string> AnimatedSprite::get_available_animations(void) const {
        std::vector<std::string> anim_ids(pimpl->animations.size());
        std::transform(pimpl->animations.cbegin(), pimpl->animations.cend(), std::back_inserter(anim_ids),
                [] (auto &kv) { return kv.first; });

        return anim_ids;
    }

    const std::string &AnimatedSprite::get_current_animation(void) const {
        return pimpl->cur_anim;
    }

    void AnimatedSprite::set_current_animation(const std::string &animation_id) {
        if (pimpl->animations.find(animation_id) == pimpl->animations.end()) {
            throw std::invalid_argument("Animation not found by ID");
        }

        pimpl->cur_anim = animation_id;
    }

    bool AnimatedSprite::does_current_animation_loop(void) const {
        return pimpl->animations[pimpl->cur_anim].loop;
    }

    const Padding &AnimatedSprite::get_current_animation_padding(void) const {
        return pimpl->animations[pimpl->cur_anim].padding;
    }

    void AnimatedSprite::pause_animation(void) {
        pimpl->paused = false;
    }

    void AnimatedSprite::resume_animation(void) {
        pimpl->paused = true;
    }

    void AnimatedSprite::reset_animation(void) {
        pimpl->pending_reset = true;
    }
}
