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

#include "argus/lowlevel/logging.hpp"

#include "argus/resman.hpp"

#include "argus/render/common/shader_compilation.hpp"

#include "internal/render_vulkan/renderer/shader_mgmt.hpp"

#include "spirv_glsl.hpp"
#include "glslang/Public/ShaderLang.h"

#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace argus {
    struct ShaderCompilationResult {
        std::vector<Shader> shaders;
        ShaderReflectionInfo reflection;
    };

    static ShaderCompilationResult _compile_glsl_shaders(const std::vector<Shader> &shaders) {
        std::vector<std::pair<Shader, VkShaderModule>> handles;

        if (shaders.empty()) {
            return ShaderCompilationResult {};
        }

        std::vector<std::string> shader_uids;
        std::vector<std::string> shader_sources;
        for (auto &shader : shaders) {
            shader_uids.emplace_back(shader.get_uid());
            shader_sources.emplace_back(shader.get_source().begin(), shader.get_source().end());
        }

        std::ostringstream shader_uids_oss;
        std::copy(shader_uids.begin(), shader_uids.end(),
                std::ostream_iterator<std::string>(shader_uids_oss, ", "));
        auto shader_uids_str = shader_uids_oss.str().substr(0, shader_uids_oss.str().size() - strlen(", "));
        Logger::default_logger().debug("Compiling SPIR-V from shader set [%s]", shader_uids_str.c_str());

        auto comp_res = compile_glsl_to_spirv(shaders, glslang::EShClientOpenGL,
                glslang::EShTargetOpenGL_450, glslang::EShTargetSpv_1_0);
        auto spirv_shaders = std::move(comp_res.first);
        auto refl_info = comp_res.second;

        ShaderCompilationResult res;
        res.reflection = refl_info;

        for (auto &shader : spirv_shaders) {
            Logger::default_logger().debug("Creating shader %s", shader.get_uid().c_str());
            res.shaders.emplace_back(shader);
        }

        return res;
    }

    PreparedShaderSet prepare_shaders(VkDevice device,
            std::initializer_list<std::pair<std::string, ShaderStage>> shader_uids) {
        return link_program(device, std::vector<std::pair<std::string, ShaderStage>>(shader_uids));
    }

    PreparedShaderSet prepare_shaders(VkDevice device,
            const std::vector<std::pair<std::string, ShaderStage>> &shader_uids) {
        PreparedShaderSet res;

        std::vector<Resource *> shader_resources;
        std::vector<Shader> loaded_shaders;
        for (const auto &shader_uid : shader_uids) {
            auto &shader_res = ResourceManager::instance().get_resource(shader_uid.first);
            auto &shader = shader_res.get<Shader>();

            shader_resources.push_back(&shader_res);
            loaded_shaders.push_back(shader);
        }

        //TODO: handle native SPIR-V shaders too
        auto comp_res = _compile_glsl_shaders(loaded_shaders);
        res.reflection = comp_res.reflection;

        for (const auto &shader : comp_res.shaders) {
            auto stage = shader.get_stage();
            auto spirv_src = shader.get_source();

            VkShaderStageFlagBits vk_shader_stage;
            switch (stage) {
                case ShaderStage::Vertex:
                    vk_shader_stage = VK_SHADER_STAGE_VERTEX_BIT;
                    break;
                case ShaderStage::Fragment:
                    vk_shader_stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                    break;
                default:
                    Logger::default_logger().fatal("Unrecognized shader stage ordinal %d",
                            static_cast<std::underlying_type<ShaderStage>::type>(stage));
            }

            VkShaderModuleCreateInfo stage_create_info{};
            stage_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            stage_create_info.codeSize = shader.get_source().size();
            stage_create_info.pCode = reinterpret_cast<const uint32_t *>(shader.get_source().data());

            VkShaderModule shader_module;
            if (vkCreateShaderModule(device, &stage_create_info, nullptr, &shader_module) != VK_SUCCESS) {
                Logger::default_logger().fatal("Failed to create shader");
            }

            VkPipelineShaderStageCreateInfo pipeline_stage_create_info;
            pipeline_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            pipeline_stage_create_info.stage = vk_shader_stage;
            pipeline_stage_create_info.module = shader_module;
            pipeline_stage_create_info.pName = "main";

            res.stages.push_back(pipeline_stage_create_info);
        }

        return res;
    }

    void destroy_shaders(VkDevice device, const PreparedShaderSet &shaders) {
        for (const auto &shader : shaders.stages) {
            vkDestroyShaderModule(device, shader.module, nullptr);
        }
    }
}
