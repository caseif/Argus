use ash::vk;
use argus_render::common::ShaderReflectionInfo;
use crate::setup::device::VulkanDevice;
use crate::util::defines::MAX_FRAMES_IN_FLIGHT;
use crate::util::VulkanBuffer;

pub(crate) struct DescriptorSetInfo {
    handle: vk::DescriptorSet,
    buffer: VulkanBuffer,
}

const INITIAL_VIEWPORT_CAP: u32 = 2;
const INITIAL_BUCKET_CAP: u32 = 64;
const SAMPLERS_PER_BUCKET: u32 = 1;
const UBOS_PER_BUCKET: u32 = 3;
const INITIAL_DS_COUNT: u32 =
    INITIAL_VIEWPORT_CAP *
    INITIAL_BUCKET_CAP *
    (MAX_FRAMES_IN_FLIGHT as u32);
const INITIAL_UBO_COUNT: u32 = INITIAL_DS_COUNT * UBOS_PER_BUCKET;
const INITIAL_SAMPLER_COUNT: u32 = INITIAL_DS_COUNT * SAMPLERS_PER_BUCKET;

fn create_ubo_bindings(shader_refl: &ShaderReflectionInfo)
    -> Vec<vk::DescriptorSetLayoutBinding> {
    shader_refl.get_ubo_bindings().iter()
        .map(|(_, ubo)| {
            vk::DescriptorSetLayoutBinding::default()
                .binding(*ubo)
                .descriptor_type(vk::DescriptorType::UNIFORM_BUFFER)
                .descriptor_count(1) //TODO: account for array UBOs
                .stage_flags(vk::ShaderStageFlags::ALL_GRAPHICS)
        })
        .collect()
}

fn create_sampler_binding<'a>() -> vk::DescriptorSetLayoutBinding<'a> {
    vk::DescriptorSetLayoutBinding::default()
        .binding(0) //TODO: pass actual value through reflection info
        .descriptor_type(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
        .descriptor_count(1)
        .stage_flags(vk::ShaderStageFlags::ALL_GRAPHICS)
}

pub(crate) fn create_descriptor_pool(device: &VulkanDevice) -> Result<vk::DescriptorPool, String> {
    let ubo_pool_size = vk::DescriptorPoolSize::default()
        .ty(vk::DescriptorType::UNIFORM_BUFFER)
        .descriptor_count(INITIAL_UBO_COUNT);

    let sampler_pool_size = vk::DescriptorPoolSize::default()
        .ty(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
        .descriptor_count(INITIAL_SAMPLER_COUNT);

    let pool_sizes = [ubo_pool_size, sampler_pool_size];

    let desc_pool_info = vk::DescriptorPoolCreateInfo::default()
        .pool_sizes(&pool_sizes)
        .max_sets(INITIAL_DS_COUNT)
        .flags(vk::DescriptorPoolCreateFlags::FREE_DESCRIPTOR_SET);

    let pool = unsafe {
        device.logical_device.create_descriptor_pool(&desc_pool_info, None)
            .map_err(|err| err.to_string())?
    };
    Ok(pool)
}

pub(crate) fn destroy_descriptor_pool(device: &VulkanDevice, pool: vk::DescriptorPool) {
    unsafe {
        device.logical_device.destroy_descriptor_pool(pool, None);
    }
}

pub(crate) fn create_descriptor_set_layout(
    device: &VulkanDevice,
    shader_refl: &ShaderReflectionInfo
) -> Result<vk::DescriptorSetLayout, String> {
    let ubo_bindings = create_ubo_bindings(shader_refl);
    let sampler_binding = create_sampler_binding();
    let mut all_bindings = ubo_bindings.clone();
    all_bindings.push(sampler_binding);

    create_descriptor_set_layout_from_bindings(device, all_bindings)
}

fn create_descriptor_set_layout_from_bindings(
    device: &VulkanDevice,
    bindings: Vec<vk::DescriptorSetLayoutBinding>
) -> Result<vk::DescriptorSetLayout, String> {
    assert!(bindings.len() <= u32::MAX as usize, "Too many descriptor set layout bindings");

    let layout_info = vk::DescriptorSetLayoutCreateInfo::default()
        .bindings(&bindings);

    let layout = unsafe {
        device.logical_device.create_descriptor_set_layout(&layout_info, None)
            .map_err(|err| err.to_string())?
    };
    Ok(layout)
}

pub(crate) fn destroy_descriptor_set_layout(
    device: &VulkanDevice,
    layout: vk::DescriptorSetLayout
) {
    unsafe { device.logical_device.destroy_descriptor_set_layout(layout, None) };
}

pub(crate) fn create_descriptor_sets(
    device: &VulkanDevice,
    pool: vk::DescriptorPool,
    shader_refl: &ShaderReflectionInfo
) -> Result<Vec<vk::DescriptorSet>, String> {
    //TODO: frames in flight
    let layout = create_descriptor_set_layout(device, shader_refl)?;
    let layouts = [layout];

    let ds_info = vk::DescriptorSetAllocateInfo::default()
        .descriptor_pool(pool)
        .set_layouts(&layouts);

    let sets = unsafe {
        device.logical_device.allocate_descriptor_sets(&ds_info)
            .map_err(|err| err.to_string())?
    };
    Ok(sets)
}

pub(crate) fn destroy_descriptor_sets(
    device: &VulkanDevice,
    pool: vk::DescriptorPool,
    sets: &Vec<vk::DescriptorSet>
) -> Result<(), String> {
    unsafe {
        device.logical_device.free_descriptor_sets(pool, &sets)
            .map_err(|err| err.to_string())
    }
}
