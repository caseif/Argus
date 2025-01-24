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

#include "argus/core/cabi/engine.h"

#include "argus/lowlevel/debug.hpp"

#include "argus/core/engine.hpp"

static argus::DeltaCallback _wrap_delta_callback(delta_callback_t callback, void *data) {
    return [callback, data](argus::TimeDelta dur) {
        argus_assert(dur.count() >= 0);
        auto dur_micros = std::chrono::duration_cast<std::chrono::microseconds>(dur);
        callback(uint64_t(dur_micros.count()), data);
    };
}

void argus_initialize_engine(void) {
    argus::initialize_engine();
}

[[noreturn]] void argus_start_engine(delta_callback_t callback, void *data) {
    argus::start_engine(_wrap_delta_callback(callback, data));
}

void argus_stop_engine(void) {
    argus::stop_engine();
}

[[noreturn]] void argus_crash(const char *msg) {
    argus::crash(msg);
}

LifecycleStage argus_get_current_lifecycle_stage(void) {
    return LifecycleStage(argus::get_current_lifecycle_stage());
}

Index argus_register_update_callback(delta_callback_t update_callback, Ordering ordering, void *data) {
    return argus::register_update_callback(_wrap_delta_callback(update_callback, data), argus::Ordering(ordering));
}

void argus_unregister_update_callback(Index id) {
    argus::unregister_update_callback(id);
}

Index argus_register_render_callback(delta_callback_t render_callback, Ordering ordering, void *data) {
    return argus::register_render_callback(_wrap_delta_callback(render_callback, data), argus::Ordering(ordering));
}

void argus_unregister_render_callback(Index id) {
    argus::unregister_render_callback(id);
}

void argus_run_on_game_thread(nullary_callback_t callback, void *data) {
    argus::run_on_game_thread([callback = std::move(callback), data = std::move(data)]() mutable {
        callback(data);
    });
}

bool argus_is_current_thread_update_thread(void) {
    return argus::is_current_thread_update_thread();
}
