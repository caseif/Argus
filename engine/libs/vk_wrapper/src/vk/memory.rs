use crate::vk;

pub const MEM_CLASS_DEVICE_RO: ash::vk::MemoryPropertyFlags = ash::vk::MemoryPropertyFlags::from_raw(
    ash::vk::MemoryPropertyFlags::DEVICE_LOCAL.as_raw()
);
pub const MEM_CLASS_DEVICE_RW: ash::vk::MemoryPropertyFlags = ash::vk::MemoryPropertyFlags::from_raw(
    ash::vk::MemoryPropertyFlags::DEVICE_LOCAL.as_raw() |
    ash::vk::MemoryPropertyFlags::HOST_VISIBLE.as_raw()
);
#[allow(dead_code)]
pub const MEM_CLASS_DEVICE_LAZY: ash::vk::MemoryPropertyFlags = ash::vk::MemoryPropertyFlags::from_raw(
    ash::vk::MemoryPropertyFlags::DEVICE_LOCAL.as_raw() |
        ash::vk::MemoryPropertyFlags::LAZILY_ALLOCATED.as_raw()
);
pub const MEM_CLASS_HOST_RW: ash::vk::MemoryPropertyFlags = ash::vk::MemoryPropertyFlags::from_raw(
    ash::vk::MemoryPropertyFlags::HOST_VISIBLE.as_raw() |
    ash::vk::MemoryPropertyFlags::HOST_COHERENT.as_raw()
);

pub fn find_memory_type(
    device: &vk::Device,
    type_filter: u32,
    allocation_size: vk::DeviceSize,
    props: ash::vk::MemoryPropertyFlags,
) -> Option<u32> {
    let mem_props = device.physical_device.get_memory_properties();

    let mut search_props = props;

    loop {
        for i in 0..mem_props.memory_type_count {
            let mem_type = mem_props.memory_types[i as usize];
            if (type_filter & (1 << i)) != 0 &&
                mem_type.property_flags.contains(props) &&
                mem_type.property_flags.contains(ash::vk::MemoryPropertyFlags::HOST_VISIBLE) &&
                !mem_type.property_flags.contains(ash::vk::MemoryPropertyFlags::DEVICE_COHERENT_AMD) &&
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
