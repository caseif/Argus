/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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
#include "argus/lowlevel/macros.hpp"

#include "argus/shadertools.hpp"

#include "argus/render/common/shader.hpp"
#include "argus/render/common/shader_compilation.hpp"

#include "glslang/Public/ShaderLang.h"
#include "SPIRV/GlslangToSpv.h"

#include <map>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace argus {
    const TBuiltInResource DefaultTBuiltInResource = {
        /* .MaxLights = */ 32,
        /* .MaxClipPlanes = */ 6,
        /* .MaxTextureUnits = */ 32,
        /* .MaxTextureCoords = */ 32,
        /* .MaxVertexAttribs = */ 64,
        /* .MaxVertexUniformComponents = */ 4096,
        /* .MaxVaryingFloats = */ 64,
        /* .MaxVertexTextureImageUnits = */ 32,
        /* .MaxCombinedTextureImageUnits = */ 80,
        /* .MaxTextureImageUnits = */ 32,
        /* .MaxFragmentUniformComponents = */ 4096,
        /* .MaxDrawBuffers = */ 32,
        /* .MaxVertexUniformVectors = */ 128,
        /* .MaxVaryingVectors = */ 8,
        /* .MaxFragmentUniformVectors = */ 16,
        /* .MaxVertexOutputVectors = */ 16,
        /* .MaxFragmentInputVectors = */ 15,
        /* .MinProgramTexelOffset = */ -8,
        /* .MaxProgramTexelOffset = */ 7,
        /* .MaxClipDistances = */ 8,
        /* .MaxComputeWorkGroupCountX = */ 65535,
        /* .MaxComputeWorkGroupCountY = */ 65535,
        /* .MaxComputeWorkGroupCountZ = */ 65535,
        /* .MaxComputeWorkGroupSizeX = */ 1024,
        /* .MaxComputeWorkGroupSizeY = */ 1024,
        /* .MaxComputeWorkGroupSizeZ = */ 64,
        /* .MaxComputeUniformComponents = */ 1024,
        /* .MaxComputeTextureImageUnits = */ 16,
        /* .MaxComputeImageUniforms = */ 8,
        /* .MaxComputeAtomicCounters = */ 8,
        /* .MaxComputeAtomicCounterBuffers = */ 1,
        /* .MaxVaryingComponents = */ 60,
        /* .MaxVertexOutputComponents = */ 64,
        /* .MaxGeometryInputComponents = */ 64,
        /* .MaxGeometryOutputComponents = */ 128,
        /* .MaxFragmentInputComponents = */ 128,
        /* .MaxImageUnits = */ 8,
        /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
        /* .MaxCombinedShaderOutputResources = */ 8,
        /* .MaxImageSamples = */ 0,
        /* .MaxVertexImageUniforms = */ 0,
        /* .MaxTessControlImageUniforms = */ 0,
        /* .MaxTessEvaluationImageUniforms = */ 0,
        /* .MaxGeometryImageUniforms = */ 0,
        /* .MaxFragmentImageUniforms = */ 8,
        /* .MaxCombinedImageUniforms = */ 8,
        /* .MaxGeometryTextureImageUnits = */ 16,
        /* .MaxGeometryOutputVertices = */ 256,
        /* .MaxGeometryTotalOutputComponents = */ 1024,
        /* .MaxGeometryUniformComponents = */ 1024,
        /* .MaxGeometryVaryingComponents = */ 64,
        /* .MaxTessControlInputComponents = */ 128,
        /* .MaxTessControlOutputComponents = */ 128,
        /* .MaxTessControlTextureImageUnits = */ 16,
        /* .MaxTessControlUniformComponents = */ 1024,
        /* .MaxTessControlTotalOutputComponents = */ 4096,
        /* .MaxTessEvaluationInputComponents = */ 128,
        /* .MaxTessEvaluationOutputComponents = */ 128,
        /* .MaxTessEvaluationTextureImageUnits = */ 16,
        /* .MaxTessEvaluationUniformComponents = */ 1024,
        /* .MaxTessPatchComponents = */ 120,
        /* .MaxPatchVertices = */ 32,
        /* .MaxTessGenLevel = */ 64,
        /* .MaxViewports = */ 16,
        /* .MaxVertexAtomicCounters = */ 0,
        /* .MaxTessControlAtomicCounters = */ 0,
        /* .MaxTessEvaluationAtomicCounters = */ 0,
        /* .MaxGeometryAtomicCounters = */ 0,
        /* .MaxFragmentAtomicCounters = */ 8,
        /* .MaxCombinedAtomicCounters = */ 8,
        /* .MaxAtomicCounterBindings = */ 1,
        /* .MaxVertexAtomicCounterBuffers = */ 0,
        /* .MaxTessControlAtomicCounterBuffers = */ 0,
        /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
        /* .MaxGeometryAtomicCounterBuffers = */ 0,
        /* .MaxFragmentAtomicCounterBuffers = */ 1,
        /* .MaxCombinedAtomicCounterBuffers = */ 1,
        /* .MaxAtomicCounterBufferSize = */ 16384,
        /* .MaxTransformFeedbackBuffers = */ 4,
        /* .MaxTransformFeedbackInterleavedComponents = */ 64,
        /* .MaxCullDistances = */ 8,
        /* .MaxCombinedClipAndCullDistances = */ 8,
        /* .MaxSamples = */ 4,
        /* .maxMeshOutputVerticesNV = */ 256,
        /* .maxMeshOutputPrimitivesNV = */ 512,
        /* .maxMeshWorkGroupSizeX_NV = */ 32,
        /* .maxMeshWorkGroupSizeY_NV = */ 1,
        /* .maxMeshWorkGroupSizeZ_NV = */ 1,
        /* .maxTaskWorkGroupSizeX_NV = */ 32,
        /* .maxTaskWorkGroupSizeY_NV = */ 1,
        /* .maxTaskWorkGroupSizeZ_NV = */ 1,
        /* .maxMeshViewCountNV = */ 4,
        /* .maxDualSourceDrawBuffersEXT = */ 1,

        /* .limits = */
        {
            /* .nonInductiveForLoops = */ 1,
            /* .whileLoops = */ 1,
            /* .doWhileLoops = */ 1,
            /* .generalUniformIndexing = */ 1,
            /* .generalAttributeMatrixVectorIndexing = */ 1,
            /* .generalVaryingIndexing = */ 1,
            /* .generalSamplerIndexing = */ 1,
            /* .generalVariableIndexing = */ 1,
            /* .generalConstantMatrixVectorIndexing = */ 1,
        }};

    std::pair<std::vector<Shader>, ShaderReflectionInfo> compile_glsl_to_spirv(
        const std::vector<Shader> &glsl_shaders, glslang::EShClient client,
        glslang::EShTargetClientVersion client_version, glslang::EShTargetLanguageVersion spirv_version) {
        glslang::InitializeProcess();

        glslang::TProgram program;

        EShMessages messages;

        std::map<std::string, const Shader*> glsl_shaders_map;

        std::map<EShLanguage, std::string> shaders_map;
        std::map<EShLanguage, std::string> shader_uids;

        for (const auto &glsl_shader : glsl_shaders) {
            Logger::default_logger().debug("Compiling shader %s to SPIR-V", glsl_shader.get_uid().c_str());

            glsl_shaders_map[glsl_shader.get_uid()] = &glsl_shader;

            auto stage = glsl_shader.get_stage();
            auto src_str = std::string(glsl_shader.get_source().begin(), glsl_shader.get_source().end());

            EShLanguage lang;
            switch (stage) {
                case ShaderStage::Vertex:
                    lang = EShLangVertex;
                    break;
                case ShaderStage::Fragment:
                    lang = EShLangFragment;
                    break;
                default:
                    throw std::invalid_argument("Unsupported shader stage");
            }

            shaders_map[lang] = src_str;
            shader_uids[lang] = glsl_shader.get_uid();
        }

        std::vector<Shader> spirv_shaders;

        auto comp_res = process_glsl(shaders_map, glslang::EShClientOpenGL, client_version, spirv_version);

        for (auto spirv_shader_kv : comp_res.spirv_shaders) {
            auto lang = spirv_shader_kv.first;
            auto uid = shader_uids[lang];
            auto spirv_u8 = spirv_shader_kv.second;

            ShaderStage stage;
            switch (lang) {
                case EShLangVertex:
                    stage = ShaderStage::Vertex;
                    break;
                case EShLangFragment:
                    stage = ShaderStage::Fragment;
                    break;
                default:
                    throw std::invalid_argument("Unsupported shader stage");
            }

            spirv_shaders.push_back(Shader(uid, SHADER_TYPE_SPIR_V, stage, spirv_u8));
        }

        for (auto shader_attr : comp_res.attributes) {
            Logger::default_logger().debug("Found shader program attribute %s @ location %d",
                shader_attr.first.c_str(), shader_attr.second);
        }

        for (auto shader_output : comp_res.outputs) {
            Logger::default_logger().debug("Found shader program output %s @ location %d",
                shader_output.first.c_str(), shader_output.second);
        }

        for (auto shader_uniform : comp_res.uniforms) {
            Logger::default_logger().debug("Found shader program uniform %s @ location %d",
                shader_uniform.first.c_str(), shader_uniform.second);
        }

        for (auto shader_buffer : comp_res.buffers) {
            Logger::default_logger().debug("Found shader program buffer %s @ location %d",
                shader_buffer.first.c_str(), shader_buffer.second);
        }

        ShaderReflectionInfo refl;
        refl.attribute_locations = comp_res.attributes;
        refl.output_locations = comp_res.outputs;
        refl.uniform_variable_locations = comp_res.uniforms;
        refl.buffer_locations = comp_res.buffers;

        return std::make_pair(spirv_shaders, refl);
    }
}
