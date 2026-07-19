use std::collections::HashMap;
use glslang::{SpirvVersion, VulkanVersion};
use argus_logging::debug;
use argus_render::common::{Shader, ShaderStage};
use argus_render::constants::*;
use argus_resman::{Resource, ResourceManager};
use argus_shadertools::{compile_glsl_to_spirv, GlslCompileError, ShaderReflectionInfo};
use vk_wrapper::vk;
use crate::LOGGER;

pub(crate) struct PreparedShaderSet<'ctx> {
    pub(crate) stages: Vec<vk::PipelineShaderStageCreateInfo<'ctx>>,
    pub(crate) reflection: ShaderReflectionInfo,
}

struct ShaderCompilationResult {
    shaders: Vec<Shader<u32>>,
    reflection: ShaderReflectionInfo,
}

fn compile_glsl_shaders(shaders: &[Shader<u8>])
    -> Result<ShaderCompilationResult, GlslCompileError> {
    if shaders.is_empty() {
        return Ok(ShaderCompilationResult {
            shaders: Vec::new(),
            reflection: ShaderReflectionInfo::default(),
        });
    }

    let uids: Vec<_> = shaders.iter().map(|shader| shader.get_uid()).collect();
    let orig_shaders_map: HashMap<_, _> =
        shaders.iter().map(|shader| (shader.get_stage(), shader)).collect();
    let glsl_sources_map = &shaders
        .iter()
        .map(|shader| {
            (
                to_shadertools_stage(&shader.get_stage()),
                String::from_utf8(shader.get_source().to_vec()).unwrap(),
            )
        })
        .collect();

    debug!(LOGGER, "Compiling SPIR-V from shader set {:?}", uids);

    let compile_res = compile_glsl_to_spirv(
        glsl_sources_map,
        glslang::Target::Vulkan {
            version: VulkanVersion::Vulkan1_2,
            spirv_version: SpirvVersion::SPIRV1_5,
        },
        450,
    )?;

    let refl_info = ShaderReflectionInfo {
        attribute_locations: compile_res.inputs,
        output_locations: compile_res.outputs,
        uniform_variable_locations: compile_res.uniforms,
        buffer_locations: compile_res.buffers,
        ubo_bindings: compile_res.ubo_bindings,
        ubo_instance_names: compile_res.ubo_names,
    };

    let mut spirv_shaders: Vec<Shader<u32>> = Vec::new();
    let mut bytecode = compile_res.bytecode;
    for (stage, orig) in &orig_shaders_map {
        let spirv_src = bytecode.remove(&to_shadertools_stage(stage)).unwrap();
        let spirv_shader = Shader::new(
            orig.get_uid(),
            orig.get_stage(),
            spirv_src,
        );
        spirv_shaders.push(spirv_shader);
    }

    Ok(ShaderCompilationResult {
        shaders: spirv_shaders,
        reflection: refl_info,
    })
}

pub(crate) fn prepare_shaders<'ctx>(device: &'ctx vk::Device, shader_uids: &[impl AsRef<str>])
                                    -> Result<PreparedShaderSet<'ctx>, String> {
    let mut shader_resources: Vec<Resource> = Vec::with_capacity(shader_uids.len());
    let mut loaded_shaders: Vec<Shader<u8>> = Vec::with_capacity(shader_uids.len());
    let mut have_vert = false;
    let mut have_frag = false;
    for shader_uid in shader_uids {
        let Ok(shader_res) = ResourceManager::instance().get_resource(shader_uid) else {
            return Err(format!("Failed to load shader {}", shader_uid.as_ref()));
        };
        let Some(shader) = shader_res.get::<Shader<u8>>() else {
            return Err("Resource UID does not refer to shader".to_string());
        };

        shader_resources.push(shader_res.clone());
        loaded_shaders.push(shader.clone());

        if shader.get_stage() == ShaderStage::Vertex {
            have_vert = true;
        } else if shader.get_stage() == ShaderStage::Fragment {
            have_frag = true;
        }
    }

    if !have_vert {
        let Ok(vert_res) = ResourceManager::instance().get_resource(SHADER_STD_VERT) else {
            return Err(format!("Failed to load built-in shader {SHADER_STD_VERT}"));
        };
        let Some(vert) = vert_res.get::<Shader<u8>>() else {
            return Err(format!(
                "Resource UID '{}' does not refer to a shader",
                vert_res.get_prototype().uid,
            ));
        };
        shader_resources.push(vert_res.clone());
        loaded_shaders.push(vert.clone());
    }
    if !have_frag {
        let Ok(frag_res) = ResourceManager::instance().get_resource(SHADER_STD_FRAG) else {
            return Err(format!("Failed to load built-in shader {}", SHADER_STD_FRAG));
        };
        let Some(frag) = frag_res.get::<Shader<u8>>() else {
            return Err(format!(
                "Resource UID '{}' does not refer to a shader",
                frag_res.get_prototype().uid,
            ));
        };
        shader_resources.push(frag_res.clone());
        loaded_shaders.push(frag.clone());
    }

    //TODO: handle native SPIR-V shaders too
    let comp_res = compile_glsl_shaders(&loaded_shaders)
        .map_err(|err| format!("Failed to compile GLSL shaders {:?}", err))?;

    let mut compiled_shaders = Vec::new();

    for shader in comp_res.shaders {
        let stage = shader.get_stage();
        let spirv_src = shader.get_source();

        let vk_shader_stage = match stage {
            ShaderStage::Vertex => vk::ShaderStageFlags::VERTEX,
            ShaderStage::Fragment => vk::ShaderStageFlags::FRAGMENT,
        };

        let stage_create_info = vk::ShaderModuleCreateInfo::default()
            .code(spirv_src);

        let shader_module = vk::ShaderModule::create(device, stage_create_info)
            .map_err(|err| err.to_string())?;

        let pipeline_stage_create_info = vk::PipelineShaderStageCreateInfo::default()
            .stage(vk_shader_stage)
            .module(shader_module)
            .name(c"main");

        compiled_shaders.push(pipeline_stage_create_info);
    }

    Ok(PreparedShaderSet {
        stages: compiled_shaders,
        reflection: comp_res.reflection,
    })
}

fn to_shadertools_stage(stage: &ShaderStage) -> glslang::ShaderStage {
    match stage {
        ShaderStage::Vertex => glslang::ShaderStage::Vertex,
        ShaderStage::Fragment => glslang::ShaderStage::Fragment,
    }
}
