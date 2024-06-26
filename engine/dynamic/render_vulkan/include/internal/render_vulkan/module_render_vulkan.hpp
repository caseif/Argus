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

#pragma once

#include "argus/core/module.hpp"

#include "internal/render_vulkan/setup/device.hpp"

#include "vulkan/vulkan.h"

#include <vector>

namespace argus {
    extern std::vector<const char *> g_engine_device_extensions;
    extern std::vector<const char *> g_engine_instance_extensions;
    extern std::vector<const char *> g_engine_layers;

    extern VkInstance g_vk_instance;
    extern LogicalDevice g_vk_device;

    extern "C" void update_lifecycle_render_vulkan(LifecycleStage stage);
}
