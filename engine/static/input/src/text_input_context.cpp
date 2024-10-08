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

#include "argus/lowlevel/collections.hpp"

#include "argus/input/text_input_context.hpp"

#include <string>
#include <vector>

namespace argus::input {
    static std::vector<TextInputContext *> g_input_contexts;
    static TextInputContext *g_active_input_context = nullptr;

    TextInputContext &TextInputContext::create_context(void) {
        return *new TextInputContext();
    }

    TextInputContext::TextInputContext(void):
        m_valid(true),
        m_active(false),
        m_text() {
        this->activate();
    }

    const std::string &TextInputContext::get_current_text(void) const {
        return m_text;
    }

    void TextInputContext::activate(void) {
        if (g_active_input_context != nullptr) {
            g_active_input_context->deactivate();
        }

        this->m_active = true;
        g_active_input_context = this;
    }

    void TextInputContext::deactivate(void) {
        if (!this->m_active) {
            return;
        }

        this->m_active = false;
        g_active_input_context = nullptr;
    }

    void TextInputContext::release(void) {
        this->deactivate();
        this->m_valid = false;
        remove_from_vector(g_input_contexts, this);
    }
}
