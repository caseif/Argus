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

#include "argus/lowlevel/time.hpp"

#include <chrono>
#include <functional>
#include <vector>

#include <cstdint>

namespace argus {
    struct BindableTimeDelta;

    typedef std::function<void(BindableTimeDelta)> ScriptDeltaCallback;

    struct BindableTimeDelta : ScriptVisible {
        uint64_t m_nanos;

        BindableTimeDelta(TimeDelta delta);

        BindableTimeDelta(const BindableTimeDelta &rhs);

        ~BindableTimeDelta(void) override;

        uint64_t nanos(void);

        uint64_t micros(void);

        uint64_t millis(void);

        uint64_t seconds(void);
    };


    void register_default_bindings(void);

    void invoke_update_callbacks(TimeDelta delta);
}
