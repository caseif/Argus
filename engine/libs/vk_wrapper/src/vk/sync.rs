use crate::vk;

pub use ash::vk::FenceCreateFlags;
pub use ash::vk::FenceCreateInfo;
pub use ash::vk::SemaphoreCreateInfo;
use crate::vk::Wrapper;

#[derive(Clone, Copy)]
pub struct Fence<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::Fence,
}

impl Wrapper for Fence<'_> {
    type Underlying = ash::vk::Fence;

    unsafe fn get_underlying(&self) -> ash::vk::Fence {
        self.underlying
    }
}

impl<'ctx> Fence<'ctx> {
    pub fn create(device: &'ctx vk::Device<'ctx>, create_info: &vk::FenceCreateInfo)
                  -> Result<Self, String> {
        let fence = unsafe {
            device.get_underlying().create_fence(create_info, None)
                .map_err(|err| err.to_string())?
        };
        Ok(Self {
            device,
            underlying: fence,
        })
    }

    pub fn destroy(self) {
        unsafe { self.device.get_underlying().destroy_fence(self.underlying, None); }
    }
}

#[derive(Clone, Copy)]
pub struct Semaphore<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::Semaphore,
}

impl<'ctx> Wrapper for Semaphore<'ctx> {
    type Underlying = ash::vk::Semaphore;
    unsafe fn get_underlying(&self) -> ash::vk::Semaphore {
        self.underlying
    }
}

impl<'ctx> Semaphore<'ctx> {
    pub fn create(device: &'ctx vk::Device<'ctx>, create_info: &vk::SemaphoreCreateInfo)
                  -> Result<Self, String> {
        let semaphore = unsafe {
            device.get_underlying().create_semaphore(create_info, None)
                .map_err(|err| err.to_string())?
        };
        Ok(Self {
            device,
            underlying: semaphore,
        })
    }

    pub fn destroy(self) {
        unsafe { self.device.get_underlying().destroy_semaphore(self.underlying, None); }
    }
}
