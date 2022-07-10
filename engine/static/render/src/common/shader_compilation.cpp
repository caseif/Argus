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

#include "argus/render/common/shader.hpp"

#include "SPIRV/GlslangToSpv.h"
#include "glslang/Public/ShaderLang.h"

#include <stdexcept>

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

    std::vector<Shader> compile_glsl_to_spirv(
        const std::vector<Shader> &glsl_shaders, glslang::EShClient client,
        glslang::EShTargetClientVersion client_version, glslang::EShTargetLanguageVersion spirv_version) {
        glslang::InitializeProcess();

        glslang::TProgram program;

        EShMessages messages;

        std::map<std::string, const Shader*> glsl_shaders_map;

        // we need to store these in the outer scope so they can be used during linking
        std::vector<glslang::TShader*> tshaders;
        std::vector<std::string*> src_strings;

        for (const auto &glsl_shader : glsl_shaders) {
            Logger::default_logger().debug("Compiling shader %s to SPIR-V", glsl_shader.get_uid().c_str());

            glsl_shaders_map[glsl_shader.get_uid()] = &glsl_shader;

            auto stage = glsl_shader.get_stage();
            auto &src_str = *new std::string(glsl_shader.get_source().begin(), glsl_shader.get_source().end());
            src_strings.push_back(&src_str);

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

            auto &shader = *new glslang::TShader(lang);
            tshaders.push_back(&shader);
            shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientOpenGL, 0);
            shader.setEnvClient(client, client_version);
            shader.setEnvTarget(glslang::EShTargetSpv, spirv_version);
            shader.setAutoMapLocations(true);
            shader.setAutoMapBindings(true);

            auto src_c = src_str.c_str();
            auto src_len = static_cast<int>(src_str.length());
            shader.setStrings(&src_c, 1);

            if (!shader.parse(&DefaultTBuiltInResource, 0, false, messages)) {
                Logger::default_logger().fatal(shader.getInfoLog());
            }

            program.addShader(&shader);
        }

        if (!program.link(messages)) {
            Logger::default_logger().fatal(program.getInfoLog());
        }

        program.buildReflection();

        std::vector<Shader> spirv_shaders;

        for (auto glsl_shader_kv : glsl_shaders_map) {
            auto &uid = glsl_shader_kv.first;
            auto &glsl_shader = *glsl_shaders_map[glsl_shader_kv.first];

            auto stage = glsl_shader.get_stage();

            EShLanguage lang;
            switch (glsl_shader.get_stage()) {
                case ShaderStage::Vertex:
                    lang = EShLangVertex;
                    break;
                case ShaderStage::Fragment:
                    lang = EShLangFragment;
                    break;
                default:
                    throw std::invalid_argument("Unsupported shader stage");
            }

            auto *intermediate = program.getIntermediate(lang);
            if (intermediate == nullptr) {
                continue;
            }

            std::vector<unsigned int> spirv_u32;
            glslang::GlslangToSpv(*intermediate, spirv_u32);
            std::vector<uint8_t> spirv_u8;
            spirv_u8.reserve(spirv_u32.size() * 4);
            for (auto i : spirv_u32) {
                spirv_u8.push_back(i & 0xFF);
                spirv_u8.push_back((i >> 8) & 0xFF);
                spirv_u8.push_back((i >> 16) & 0xFF);
                spirv_u8.push_back((i >> 24) & 0xFF);
            }

            // this will probably need to be more nuanced if we want to support additional shader stages in the future
            auto attr_count = stage == ShaderStage::Vertex ? program.getNumLiveAttributes() : 0;
            auto output_count = stage == ShaderStage::Fragment ? program.getNumPipeOutputs() : 0;
            auto uni_count = program.getNumLiveUniformVariables();

            ShaderReflectionInfo reflection{};

            for (int i = 0; i < attr_count; i++) {
                auto attr_name = program.getAttributeName(i);
                if (reflection.attribute_locations.find(attr_name) == reflection.attribute_locations.end()) {
                    auto attr_info = program.getPipeInput(i);
                    attr_info.dump();
                    reflection.attribute_locations[attr_name] = i;
                }
            }

            spirv_shaders.push_back(Shader(uid, SHADER_TYPE_SPIR_V, stage, spirv_u8, &reflection));
        }

        return spirv_shaders;
    }
}
