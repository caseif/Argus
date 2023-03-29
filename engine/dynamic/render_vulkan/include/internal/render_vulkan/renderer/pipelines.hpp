#pragma once

#include "argus/render/common/material.hpp"

#include "vulkan/vulkan.h"

namespace argus {
    // forward declarations
    struct RendererState;

    struct PipelineInfo {
        VkPipeline pipeline;
        VkPipelineLayout layout;
    };

    PipelineInfo get_or_create_pipeline(RendererState &state, const std::string &material_uid);

    void destroy_pipeline(PipelineInfo pipeline);
}
