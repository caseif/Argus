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

#include "argus/lowlevel/memory.hpp"

#include "argus/core/engine.hpp"

#include "argus/game2d/sprite.hpp"
#include "internal/game2d/pimpl/sprite.hpp"
#include "internal/game2d/sprite.hpp"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

namespace argus {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_Sprite));

    Sprite::Sprite(const Resource &definition):
        m_pimpl(&g_pimpl_pool.construct<pimpl_Sprite>(definition)) {
        set_current_animation(m_pimpl->get_def().def_anim);
    }

    Sprite::Sprite(Sprite &&rhs) noexcept:
        m_pimpl(rhs.m_pimpl) {
        rhs.m_pimpl = nullptr;
    }

    Sprite::~Sprite(void) {
        g_pimpl_pool.destroy(m_pimpl);
    }

    float Sprite::get_animation_speed(void) const {
        return m_pimpl->speed;
    }

    void Sprite::set_animation_speed(float speed) {
        m_pimpl->speed = speed;
    }

    std::vector<std::string> Sprite::get_available_animations(void) const {
        std::vector<std::string> anim_ids(m_pimpl->get_def().animations.size());
        std::transform(m_pimpl->get_def().animations.cbegin(), m_pimpl->get_def().animations.cend(),
                std::back_inserter(anim_ids), [](auto &kv) { return kv.first; });

        return anim_ids;
    }

    const std::string &Sprite::get_current_animation(void) const {
        return m_pimpl->cur_anim_id;
    }

    void Sprite::set_current_animation(const std::string &animation_id) {
        auto it = m_pimpl->get_def().animations.find(animation_id);
        if (it == m_pimpl->get_def().animations.end()) {
            crash("Animation not found by ID");
        }

        m_pimpl->cur_anim_id = animation_id;
        //TODO: revisit this
        m_pimpl->cur_anim = const_cast<SpriteAnimation *>(&it->second);
        m_pimpl->cur_frame = m_pimpl->anim_start_offsets[m_pimpl->cur_anim_id];
    }

    bool Sprite::does_current_animation_loop(void) const {
        return m_pimpl->cur_anim->loop;
    }

    bool Sprite::is_current_animation_static(void) const {
        return m_pimpl->cur_anim->frames.size() == 1;
    }

    Padding Sprite::get_current_animation_padding(void) const {
        return m_pimpl->cur_anim->padding;
    }

    void Sprite::pause_animation(void) {
        m_pimpl->paused = false;
    }

    void Sprite::resume_animation(void) {
        m_pimpl->paused = true;
    }

    void Sprite::reset_animation(void) {
        m_pimpl->pending_reset = true;
    }
}
