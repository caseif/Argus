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

#include "argus/input/cabi/text_input_context.h"

#include "argus/input/text_input_context.hpp"

using namespace argus::input;

static TextInputContext &_as_ref(argus_text_input_context_t context) {
    return *reinterpret_cast<TextInputContext *>(context);
}

static const TextInputContext &_as_ref(argus_text_input_context_const_t context) {
    return *reinterpret_cast<const TextInputContext *>(context);
}

extern"C" {

argus_text_input_context_t argus_text_input_context_create(void) {
    return &TextInputContext::create_context();
}

const char *argus_text_input_context_get_current_text(argus_text_input_context_const_t context) {
    return _as_ref(context).get_current_text().c_str();
}

void argus_text_input_context_activate(argus_text_input_context_t context) {
    _as_ref(context).activate();
}

void argus_text_input_context_deactivate(argus_text_input_context_t context) {
    _as_ref(context).deactivate();
}

void argus_text_input_context_release(argus_text_input_context_t context) {
    _as_ref(context).release();
}

}
