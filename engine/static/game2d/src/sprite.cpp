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

#include "argus/lowlevel/memory.hpp"

#include "argus/render/common/transform.hpp"

#include "argus/game2d/sprite.hpp"
#include "internal/game2d/pimpl/sprite.hpp"
#include "internal/game2d/sprite.hpp"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_Sprite));

    Sprite::Sprite(const std::string &id, const Resource &definition):
            pimpl(&g_pimpl_pool.construct<pimpl_Sprite>(id, definition)) {
        set_current_animation(pimpl->get_def().def_anim);
    }

    Sprite::Sprite(Sprite &&rhs):
            pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Sprite::~Sprite(void) {
        g_pimpl_pool.destroy(pimpl);
    }

    const std::string &Sprite::get_id(void) const {
        return pimpl->id;
    }

    const Vector2f &Sprite::get_base_size(void) const {
        return pimpl->get_def().base_size;
    }

    const Transform2D &Sprite::get_transform(void) const {
        return pimpl->transform;
    }

    void Sprite::set_transform(const Transform2D &transform) {
        pimpl->transform = transform;
        pimpl->transform_dirty = true;
    }

    float Sprite::get_animation_speed(void) const {
        return pimpl->speed;
    }

    void Sprite::set_animation_speed(float speed) {
        pimpl->speed = speed;
    }

    std::vector<std::string> Sprite::get_available_animations(void) const {
        std::vector<std::string> anim_ids(pimpl->get_def().animations.size());
        std::transform(pimpl->get_def().animations.cbegin(), pimpl->get_def().animations.cend(),
                std::back_inserter(anim_ids), [] (auto &kv) { return kv.first; });

        return anim_ids;
    }

    const std::string &Sprite::get_current_animation(void) const {
        return pimpl->cur_anim_id;
    }

    void Sprite::set_current_animation(const std::string &animation_id) {
        auto it = pimpl->get_def().animations.find(animation_id);
        if (it == pimpl->get_def().animations.end()) {
            throw std::invalid_argument("Animation not found by ID");
        }

        pimpl->cur_anim_id = animation_id;
        //TODO: revisit this
        pimpl->cur_anim = const_cast<SpriteAnimation*>(&it->second);
        pimpl->cur_frame = pimpl->anim_start_offsets[pimpl->cur_anim_id];
    }

    bool Sprite::does_current_animation_loop(void) const {
        return pimpl->cur_anim->loop;
    }

    const Padding &Sprite::get_current_animation_padding(void) const {
        return pimpl->cur_anim->padding;
    }

    void Sprite::pause_animation(void) {
        pimpl->paused = false;
    }

    void Sprite::resume_animation(void) {
        pimpl->paused = true;
    }

    void Sprite::reset_animation(void) {
        pimpl->pending_reset = true;
    }
}
