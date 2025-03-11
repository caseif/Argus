use crate::setup::device::VulkanDevice;
use crate::state::{SubmitCommandBufferParams, SubmitMessage};
use argus_util::semaphore::Semaphore;
use ash::vk;
use std::sync::{mpsc, Mutex};
use std::sync::mpsc::SendError;

#[derive(Clone)]
pub(crate) struct CommandBufferInfo {
    handle: vk::CommandBuffer,
    pool: vk::CommandPool,
}

impl CommandBufferInfo {
    /// SAFETY: The returned handle must not outlive `self`.
    pub(crate) unsafe fn get_handle(&self) -> vk::CommandBuffer {
        self.handle
    }
}

pub(crate) fn create_command_pool(device: &VulkanDevice, queue_index: u32) -> vk::CommandPool {
    let pool_info = vk::CommandPoolCreateInfo::default()
        .flags(vk::CommandPoolCreateFlags::RESET_COMMAND_BUFFER)
        .queue_family_index(queue_index);

    let command_pool = unsafe {
        device.logical_device.create_command_pool(&pool_info, None)
            .expect("Failed to create command pool")
    };

    command_pool
}

pub(crate) fn destroy_command_pool(device: &VulkanDevice, command_pool: vk::CommandPool) {
    unsafe { device.logical_device.destroy_command_pool(command_pool, None) };
}

pub(crate) fn alloc_command_buffers(
    device: &VulkanDevice,
    pool: vk::CommandPool,
    count: u32,
) -> Vec<CommandBufferInfo> {
    let cb_alloc_info = vk::CommandBufferAllocateInfo::default()
        .command_pool(pool)
        .level(vk::CommandBufferLevel::PRIMARY)
        .command_buffer_count(count);

    let handles = unsafe {
        device.logical_device.allocate_command_buffers(&cb_alloc_info)
            .expect("Failed to allocate command buffers")
    };

    let buffers = handles.into_iter()
        .map(|handle| CommandBufferInfo {
            handle,
            pool,
        })
        .collect();
    buffers
}

pub(crate) fn free_command_buffers(device: &VulkanDevice, buffers: Vec<CommandBufferInfo>) {
    if buffers.is_empty() {
        return;
    }

    let handles: Vec<_> = buffers.iter().map(|buffer| buffer.handle).collect();
    unsafe { device.logical_device.free_command_buffers(buffers.first().unwrap().pool, &handles) };
}

pub(crate) fn destroy_command_buffer(device: &VulkanDevice, buffer: CommandBufferInfo) {
    unsafe { device.logical_device.free_command_buffers(buffer.pool, &[buffer.handle]); }
}

pub(crate) fn begin_oneshot_commands(device: &VulkanDevice, buffer: &CommandBufferInfo) {
    unsafe {
        device.logical_device
            .reset_command_buffer(buffer.handle, vk::CommandBufferResetFlags::empty())
            .expect("Failed to reset command buffer");
    }

    let begin_info = vk::CommandBufferBeginInfo::default()
        .flags(vk::CommandBufferUsageFlags::ONE_TIME_SUBMIT);
    unsafe {
        device.logical_device.begin_command_buffer(buffer.handle, &begin_info)
            .expect("Failed to begin command buffer"); //TODO
    };
}

pub(crate) fn end_command_buffer(device: &VulkanDevice, buffer: &CommandBufferInfo) {
    unsafe {
        device.logical_device.end_command_buffer(buffer.handle)
            .expect("Failed to end command buffer"); //TODO
    }
}

pub(crate) fn submit_command_buffer(
    device: &ash::Device,
    buffer: &CommandBufferInfo,
    queue: vk::Queue,
    fence: vk::Fence,
    wait_semaphores: Vec<vk::Semaphore>,
    wait_stages: Vec<vk::PipelineStageFlags>,
    signal_semaphores: Vec<vk::Semaphore>,
) {
    assert_eq!(wait_semaphores.len(), wait_stages.len());
    
    let buffers = [buffer.handle];

    let submit_info = vk::SubmitInfo::default()
        .command_buffers(&buffers)
        .wait_semaphores(&wait_semaphores)
        .wait_dst_stage_mask(&wait_stages)
        .signal_semaphores(&signal_semaphores);
    let submit_infos = [submit_info];
    unsafe {
        device.queue_submit(queue, &submit_infos, fence)
            .expect("Failed to submit command queues");
    }
}

pub(crate) fn queue_command_buffer_submit(
    sender: &mpsc::Sender<SubmitMessage>,
    submit_mutex: &Mutex<()>,
    buffer: CommandBufferInfo,
    queue: vk::Queue,
    wait_semaphores: Vec<vk::Semaphore>,
    wait_stages: Vec<vk::PipelineStageFlags>,
    signal_semaphores: Vec<vk::Semaphore>,
    fence: Option<vk::Fence>,
    in_flight_sem: Option<Semaphore>,
) -> Result<(), SendError<SubmitMessage>> {
    let _lock = submit_mutex.lock().unwrap();
    let params = {
        SubmitCommandBufferParams {
            buffer,
            queue,
            wait_sems: wait_semaphores,
            wait_stages,
            signal_sems: signal_semaphores,
            fence,
            in_flight_sem,
        }
    };
    sender.send(SubmitMessage::SubmitCommandBuffer(params))?;
    Ok(())
}
