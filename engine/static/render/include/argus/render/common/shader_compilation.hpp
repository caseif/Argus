#pragma once

#include "argus/render/common/shader.hpp"

#include "glslang/Public/ShaderLang.h"

#include <vector>

namespace argus {
    std::pair<std::vector<Shader>, ShaderReflectionInfo> compile_glsl_to_spirv(
            const std::vector<Shader> &glsl_sources,
            glslang::EShClient client,
            glslang::EShTargetClientVersion client_version,
            glslang::EShTargetLanguageVersion spirv_version
    );
}
