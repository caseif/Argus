use std::ffi;
pub use ash::vk::DebugUtilsMessageSeverityFlagsEXT;
pub use ash::vk::DebugUtilsMessageTypeFlagsEXT;

use std::ffi::CStr;
use crate::vk;
use crate::vk::Wrapper;

pub trait DebugCallback:
(Fn(DebugUtilsMessageSeverityFlagsEXT, DebugUtilsMessageTypeFlagsEXT, &str) -> u32) +
'static +
Send +
Sync {
}

impl<T:
    Fn(DebugUtilsMessageSeverityFlagsEXT, DebugUtilsMessageTypeFlagsEXT, &str) -> u32 +
    'static +
    Send +
    Sync
> DebugCallback for T {
}

pub struct DebugUtilsMessenger<'inst> {
    instance: &'inst vk::Instance,
    underlying: ash::vk::DebugUtilsMessengerEXT,
    _callback: Box<Box<dyn DebugCallback>>,
    destroyed: bool,
}

impl<'inst> Wrapper for DebugUtilsMessenger<'inst> {
    type Underlying = ash::vk::DebugUtilsMessengerEXT;

    unsafe fn get_underlying(&self) -> Self::Underlying {
        self.underlying
    }
}

impl<'inst> Drop for DebugUtilsMessenger<'inst> {
    fn drop(&mut self) {
        assert!(self.destroyed);
    }
}

impl<'inst> DebugUtilsMessenger<'inst> {
    pub fn create(
        instance: &'inst vk::Instance,
        message_severities: DebugUtilsMessageSeverityFlagsEXT,
        message_types: DebugUtilsMessageTypeFlagsEXT,
        callback: impl DebugCallback,
    ) -> Result<Self, String> {
        let callback_boxed = Box::new(Box::new(callback) as Box<dyn DebugCallback>);

        let debug_info = ash::vk::DebugUtilsMessengerCreateInfoEXT::default()
            .message_severity(message_severities)
            .message_type(message_types)
            .pfn_user_callback(Some(debug_trampoline))
            .user_data(&*callback_boxed as *const Box<dyn DebugCallback> as *mut ffi::c_void);

        let messenger = unsafe {
            instance.ext_debug_utils().create_debug_utils_messenger(&debug_info, None)
                .map_err(|err| err.to_string())?
        };

        Ok(Self {
            instance,
            underlying: messenger,
            _callback: callback_boxed,
            destroyed: false,
        })
    }

    pub fn destroy(mut self) {
        unsafe {
            self.instance.ext_debug_utils().destroy_debug_utils_messenger(self.underlying, None);
        }

        self.destroyed = true;
    }
}

unsafe extern "system" fn debug_trampoline(
    message_severity: DebugUtilsMessageSeverityFlagsEXT,
    message_type: DebugUtilsMessageTypeFlagsEXT,
    callback_data: *const ash::vk::DebugUtilsMessengerCallbackDataEXT,
    user_data: *mut ffi::c_void
) -> u32 {
    let message = unsafe { CStr::from_ptr((*callback_data).p_message) }.to_string_lossy();
    let callback: &dyn DebugCallback = unsafe { &**user_data.cast::<*const dyn DebugCallback>() };

    callback(message_severity, message_type, &message)
}
