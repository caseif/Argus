use ash::vk;
use crate::setup::device::VulkanDevice;
use crate::setup::instance::VulkanInstance;

pub(crate) const MEM_CLASS_DEVICE_RO: vk::MemoryPropertyFlags = vk::MemoryPropertyFlags::from_raw(
    vk::MemoryPropertyFlags::DEVICE_LOCAL.as_raw()
);
pub(crate) const MEM_CLASS_DEVICE_RW: vk::MemoryPropertyFlags = vk::MemoryPropertyFlags::from_raw(
    vk::MemoryPropertyFlags::DEVICE_LOCAL.as_raw() |
    vk::MemoryPropertyFlags::HOST_VISIBLE.as_raw()
);
pub(crate) const MEM_CLASS_DEVICE_LAZY: vk::MemoryPropertyFlags = vk::MemoryPropertyFlags::from_raw(
    vk::MemoryPropertyFlags::DEVICE_LOCAL.as_raw() |
    vk::MemoryPropertyFlags::LAZILY_ALLOCATED.as_raw()
);
pub(crate) const MEM_CLASS_HOST_RW: vk::MemoryPropertyFlags = vk::MemoryPropertyFlags::from_raw(
    vk::MemoryPropertyFlags::HOST_VISIBLE.as_raw() |
    vk::MemoryPropertyFlags::HOST_COHERENT.as_raw()
);

pub(crate) fn find_memory_type(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    type_filter: u32,
    allocation_size: vk::DeviceSize,
    props: vk::MemoryPropertyFlags,
) -> Option<u32> {
    let mem_props = unsafe {
        instance.get_underlying().get_physical_device_memory_properties(device.physical_device)
    };

    let mut search_props = props;

    loop {
        for i in 0..mem_props.memory_type_count {
            let mem_type = mem_props.memory_types[i as usize];
            if (type_filter & (1 << i)) != 0 &&
                mem_type.property_flags.contains(props) &&
                mem_type.property_flags.contains(vk::MemoryPropertyFlags::HOST_VISIBLE) &&
                !mem_type.property_flags.contains(vk::MemoryPropertyFlags::DEVICE_COHERENT_AMD) &&
                allocation_size <= mem_props.memory_heaps[mem_type.heap_index as usize].size {
                return Some(i);
            }
        }

        if search_props == MEM_CLASS_DEVICE_RW {
            search_props = MEM_CLASS_HOST_RW;
            continue;
        }

        break;
    }

    None
}
