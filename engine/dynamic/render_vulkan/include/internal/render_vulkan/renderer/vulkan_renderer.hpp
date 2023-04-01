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

#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"

#include "argus/core/engine.hpp"

#include "argus/wm/window.hpp"

#include "internal/render_vulkan/setup/swapchain.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"

#include "vulkan/vulkan.h"

namespace argus {
    class VulkanRenderer {
       private:
        Index resource_event_handler;

       public:
        Window &window;
        RendererState state;

        VulkanRenderer(Window &window);

        VulkanRenderer(const VulkanRenderer &) = delete;

        VulkanRenderer(VulkanRenderer &&) = delete;

        ~VulkanRenderer(void);

        void render(TimeDelta delta);

        void notify_window_resize(const Vector2u &resolution);
    };
}
