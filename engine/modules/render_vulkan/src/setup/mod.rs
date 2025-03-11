use argus_logging::crate_logger;

pub(crate) mod device;
pub(crate) mod instance;
pub(crate) mod queues;
pub(crate) mod swapchain;

crate_logger!(LOGGER, "argus/render_vulkan");
