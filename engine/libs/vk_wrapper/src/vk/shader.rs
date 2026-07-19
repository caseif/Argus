use std::ffi::CString;
use crate::vk;

pub use ash::vk::ShaderModuleCreateInfo;
pub use ash::vk::ShaderStageFlags;
use crate::vk::Wrapper;

pub struct ShaderModule<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::ShaderModule,
}

impl<'ctx> Wrapper for ShaderModule<'ctx> {
    type Underlying = ash::vk::ShaderModule;

    unsafe fn get_underlying(&self) -> ash::vk::ShaderModule {
        self.underlying
    }
}

impl<'ctx> ShaderModule<'ctx> {
    pub fn create(device: &'ctx vk::Device<'ctx>, create_info: vk::ShaderModuleCreateInfo)
                  -> Result<ShaderModule<'ctx>, String> {
        let shader_mod = unsafe {
            device.get_underlying().create_shader_module(&create_info, None)
                .map_err(|err| err.to_string())?
        };
        Ok(Self {
            device,
            underlying: shader_mod,
        })
    }

    pub fn destroy(self) {
        unsafe {
            self.device.get_underlying().destroy_shader_module(self.underlying, None);
        }
    }
}

#[derive(Default)]
pub struct PipelineShaderStageCreateInfo<'ctx> {
    flags: ash::vk::PipelineShaderStageCreateFlags,
    stage: Option<ShaderStageFlags>,
    module: Option<ShaderModule<'ctx>>,
    name: Option<CString>,
}

impl<'ctx> PipelineShaderStageCreateInfo<'ctx> {
    pub fn flags(mut self, flags: ash::vk::PipelineShaderStageCreateFlags) -> Self {
        self.flags = flags;
        self
    }

    pub fn stage(mut self, stage: ShaderStageFlags) -> Self {
        self.stage = Some(stage);
        self
    }

    pub fn module(mut self, shader_module: ShaderModule<'ctx>) -> Self {
        self.module = Some(shader_module);
        self
    }

    pub fn name(mut self, name: impl Into<CString>) -> Self {
        self.name = Some(name.into());
        self
    }

    pub(crate) fn create_underlying(&'_ self) -> ash::vk::PipelineShaderStageCreateInfo<'_> {
        ash::vk::PipelineShaderStageCreateInfo::default()
            .flags(self.flags)
            .stage(self.stage.unwrap())
            .module(unsafe { self.module.as_ref().unwrap().get_underlying() })
            .name(self.name.as_ref().unwrap().as_c_str())
    }
    
    pub fn destroy(self) {
        if let Some(module) = self.module {
            module.destroy();
        }
    }
}
