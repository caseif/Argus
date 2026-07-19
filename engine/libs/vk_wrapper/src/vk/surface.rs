use ash::vk::Handle;
use crate::vk;
use crate::vk::Wrapper;

pub struct Surface<'inst> {
    instance: &'inst vk::Instance,
    underlying: ash::vk::SurfaceKHR,
}

impl<'inst> Wrapper for Surface<'inst> {
    type Underlying = ash::vk::SurfaceKHR;

    unsafe fn get_underlying(&self) -> ash::vk::SurfaceKHR {
        self.underlying
    }
}

impl<'inst> Surface<'inst> {
    /// Creates a new Surface object to wrap the provided raw surface handle.
    ///
    /// # Safety
    /// `handle` must be a pointer to a valid VkSurfaceKHR which thereafter must
    /// not be used in any capacity except via the returned wrapper object.
    pub unsafe fn from_handle(instance: &'inst vk::Instance, handle: u64) -> Self {
        Self { instance, underlying: ash::vk::SurfaceKHR::from_raw(handle) }
    }

    pub fn destroy(self) {
        unsafe {
            self.instance.khr_surface().destroy_surface(self.underlying, None);
        }
    }

    pub fn get_instance(&self) -> &vk::Instance {
        self.instance
    }

    pub fn get_physical_device_support(&self, device: &vk::PhysicalDevice, i: u32)
                                       -> Result<bool, String> {
        unsafe {
            self.instance.khr_surface()
                .get_physical_device_surface_support(device.get_underlying(), i, self.underlying)
                .map_err(|err| err.to_string())
        }
    }

    pub(crate) fn get_physical_device_capabilities(&self, device: &vk::PhysicalDevice)
        -> Result<ash::vk::SurfaceCapabilitiesKHR, String> {
        unsafe {
            self.instance.khr_surface()
                .get_physical_device_surface_capabilities(device.get_underlying(), self.underlying)
                .map_err(|err| err.to_string())
        }
    }

    pub(crate) fn get_physical_device_formats(&self, device: &vk::PhysicalDevice)
                                            -> Result<Vec<ash::vk::SurfaceFormatKHR>, String> {
        unsafe {
            self.instance.khr_surface()
                .get_physical_device_surface_formats(device.get_underlying(), self.underlying)
                .map_err(|err| err.to_string())
        }
    }

    pub(crate) fn get_physical_device_present_modes(&self, device: &vk::PhysicalDevice)
                                            -> Result<Vec<ash::vk::PresentModeKHR>, String> {
        unsafe {
            self.instance.khr_surface()
                .get_physical_device_surface_present_modes(device.get_underlying(), self.underlying)
                .map_err(|err| err.to_string())
        }
    }
}
