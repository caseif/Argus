#pragma once

#include "argus/resman.hpp"

#include "internal/render_vulkan/renderer/pipelines.hpp"
#include "internal/render_vulkan/renderer/vulkan_renderer.hpp"

#include "vulkan/vulkan.h"

#include <map>
#include <string>

namespace argus {
    struct RendererState {
        VulkanRenderer &renderer;

        VkDevice device;

        Vector2u viewport_size;

        std::map<std::string, const Resource*> material_resources;
        std::map<std::string, PipelineInfo> material_pipelines;

        RendererState(VulkanRenderer &renderer);
    };
}
