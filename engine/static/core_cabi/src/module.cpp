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

#include "argus/core_cabi/module.h"

#include "argus/core/module.hpp"

const char *argus_lifecycle_stage_to_str(LifecycleStage stage) {
    return argus::lifecycle_stage_to_str(argus::LifecycleStage(stage));
}

void argus_register_dynamic_module(const char *id, void(*lifecycle_callback)(LifecycleStage)) {
    argus::register_dynamic_module(id, reinterpret_cast<argus::LifecycleUpdateCallbackPtr>(lifecycle_callback));
}

bool argus_enable_dynamic_module(const char *module_id) {
    return argus::enable_dynamic_module(module_id);
}
