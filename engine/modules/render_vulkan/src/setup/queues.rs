use std::sync::{Arc, Mutex};
use ash::vk::Queue;

#[derive(Clone, Copy, Debug)]
pub(crate) struct QueueFamilyIndices {
    pub(crate) graphics_family: u32,
    pub(crate) transfer_family: u32,
    pub(crate) present_family: u32,
}

#[derive(Clone, Copy, Debug)]
pub(crate) struct QueueFamilies {
    pub(crate) graphics_family: Queue,
    pub(crate) transfer_family: Queue,
    pub(crate) present_family: Queue,
}

#[derive(Clone, Debug, Default)]
pub(crate) struct QueueMutexes {
    pub(crate) graphics_family: Arc<Mutex<()>>,
    pub(crate) transfer_family: Arc<Mutex<()>>,
    pub(crate) present_family: Arc<Mutex<()>>,
}
