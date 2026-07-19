use crate::vk;

pub use ash::vk::BorderColor;
pub use ash::vk::Filter;
pub use ash::vk::SamplerAddressMode;
pub use ash::vk::SamplerCreateInfo;
pub use ash::vk::SamplerMipmapMode;
use crate::vk::Wrapper;

pub struct Sampler<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::Sampler,
}

impl<'ctx> Wrapper for Sampler<'ctx> {
    type Underlying = ash::vk::Sampler;

    unsafe fn get_underlying(&self) -> ash::vk::Sampler {
        self.underlying
    }
}

impl<'ctx> Sampler<'ctx> {
    pub fn create(device: &'ctx vk::Device<'ctx>, create_info: &vk::SamplerCreateInfo)
                  -> Result<Self, String> {
        let sampler = unsafe {
            device.get_underlying().create_sampler(create_info, None)
                .map_err(|err| err.to_string())
        }?;

        Ok(Self {
            device,
            underlying: sampler,
        })
    }

    pub fn destroy(self) {
        unsafe { self.device.get_underlying().destroy_sampler(self.underlying, None); }
    }
}
