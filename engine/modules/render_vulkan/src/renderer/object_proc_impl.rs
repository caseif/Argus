use ash::vk;
use argus_render::common::{Material, Matrix4x4};
use argus_render::constants::*;
use argus_render::twod::{get_render_context_2d, RenderObject2d};
use argus_util::math::Vector4f;
use argus_util::pool::Handle;
use crate::setup::device::VulkanDevice;
use crate::setup::instance::VulkanInstance;
use crate::state::{ProcessedObject, RendererState, Scene2dState};
use crate::util::{create_pipeline_for_material, PipelineInfo, VulkanBuffer, MEM_CLASS_DEVICE_RW};
use crate::util::defines::*;

pub(crate) fn process_object(
    scene_id: &str,
    object_handle: Handle,
    transform: &Matrix4x4,
    is_transform_dirty: bool,
    state_tuple: &mut (&VulkanInstance, &VulkanDevice, &mut RendererState),
) {
    let (instance, device, state) = state_tuple;
    
    let mut object = get_render_context_2d().get_object_mut(object_handle)
        .expect("Object not present in render context during processing");

    let existing_obj = {
        let scene_state = state.scene_states_2d.entry(scene_id.to_owned()).or_insert_with(|| {
            Scene2dState::new(scene_id.to_owned())
        });
        scene_state.processed_objs.get_mut(&object_handle)
    };

    if let Some(proc_obj) = existing_obj {
        // pipeline should be created by now
        let pipeline = &state.material_pipelines[&object.get_material().get_prototype().uid];
        update_processed_object_2d(
            device,
            &mut object,
            proc_obj,
            transform,
            is_transform_dirty,
            pipeline,
        );
    } else {
        let new_proc_obj =
            create_processed_object_2d(instance, device, state, &mut object, transform);
        state.scene_states_2d.get_mut(&scene_id.to_string()).unwrap().processed_objs.insert(
            object_handle.clone(),
            new_proc_obj,
        );
    }
}

pub(crate) fn create_processed_object_2d(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    state: &mut RendererState,
    object: &mut RenderObject2d,
    transform: &Matrix4x4,
) -> ProcessedObject {
    let vertex_count = object.get_primitives().iter()
        .map(|prim| prim.get_vertices().len())
        .sum::<usize>() as u32;

    let obj_mat = object.get_material().upgrade().unwrap();
    let obj_mat_uid = &obj_mat.get_prototype().uid;

    let pipeline = state.material_pipelines.entry(obj_mat_uid.clone()).or_insert_with(|| {
        create_pipeline_for_material(
            device,
            obj_mat.get::<Material>().unwrap(),
            &state.viewport_size,
            state.fb_render_pass.unwrap(),
        ).unwrap()
    });

    let vertex_comps = pipeline.vertex_len / size_of::<f32>() as u32;

    let buffer_size = vertex_count * vertex_comps * size_of::<f32>() as u32;

    assert!(buffer_size <= i32::MAX as u32, "Buffer size is too big");

    let mut staging_buffer = VulkanBuffer::new(
        instance,
        device,
        buffer_size as vk::DeviceSize,
        vk::BufferUsageFlags::TRANSFER_SRC,
        MEM_CLASS_DEVICE_RW,
    ).unwrap();

    let mut total_vertices = 0;
    
    #[allow(unused_assignments)]
    {
        let mut staging_buf_map =
            staging_buffer.map(device, 0, vk::WHOLE_SIZE, vk::MemoryMapFlags::empty()).unwrap();
        let float_buffer = staging_buf_map.as_slice_mut();

        for prim in object.get_primitives() {
            for vertex in prim.get_vertices() {
                let major_off = (total_vertices * vertex_comps) as usize;
                let mut minor_off = 0usize;

                if pipeline.reflection.has_attr(SHADER_ATTRIB_POSITION) {
                    let pos_vec = Vector4f::new(vertex.position.x, vertex.position.y, 0.0, 1.0);
                    let transformed_pos = transform * pos_vec;
                    float_buffer[major_off + minor_off] = transformed_pos.x;
                    float_buffer[major_off + minor_off + 1] = transformed_pos.y;
                    minor_off += 2;
                }
                if pipeline.reflection.has_attr(SHADER_ATTRIB_NORMAL) {
                    float_buffer[major_off + minor_off] = vertex.normal.x;
                    float_buffer[major_off + minor_off + 1] = vertex.normal.y;
                    minor_off += 2;
                }
                if pipeline.reflection.has_attr(SHADER_ATTRIB_COLOR) {
                    float_buffer[major_off + minor_off] = vertex.color.x;
                    float_buffer[major_off + minor_off + 1] = vertex.color.y;
                    float_buffer[major_off + minor_off + 2] = vertex.color.z;
                    float_buffer[major_off + minor_off + 3] = vertex.color.w;
                    minor_off += 4;
                }
                if pipeline.reflection.has_attr(SHADER_ATTRIB_TEXCOORD) {
                    float_buffer[major_off + minor_off] = vertex.tex_coord.x;
                    float_buffer[major_off + minor_off + 1] = vertex.tex_coord.y;
                    minor_off += 2;
                }

                total_vertices += 1;
            }
        }
    }

    let mut processed_obj = ProcessedObject::new(
        obj_mat,
        object.get_atlas_stride(),
        object.get_z_index(),
        object.get_light_opacity().value,
        total_vertices,
    );

    processed_obj.staging_buffer = Some(staging_buffer);

    processed_obj.anim_frame = object.get_active_frame().value;

    processed_obj.visited = true;
    processed_obj.newly_created = true;

    processed_obj
}

pub(crate) fn update_processed_object_2d(
    device: &VulkanDevice,
    object: &mut RenderObject2d,
    proc_obj: &mut ProcessedObject,
    transform: &Matrix4x4,
    is_transform_dirty: bool,
    pipeline: &PipelineInfo,
) {
    // if a parent group or the object itself has had its transform updated
    proc_obj.updated = is_transform_dirty;

    let cur_frame = object.get_active_frame();
    if cur_frame.dirty {
        proc_obj.anim_frame = cur_frame.value;
        proc_obj.anim_frame_updated = true;
    }

    if !is_transform_dirty {
        // nothing to do
        proc_obj.visited = true;
        return;
    }

    let vertex_comps = 
        pipeline.reflection.get_attr_loc(SHADER_ATTRIB_POSITION)
            .map(|_| SHADER_ATTRIB_POSITION_LEN).unwrap_or(0) +
        pipeline.reflection.get_attr_loc(SHADER_ATTRIB_NORMAL)
            .map(|_| SHADER_ATTRIB_NORMAL_LEN).unwrap_or(0) +
        pipeline.reflection.get_attr_loc(SHADER_ATTRIB_COLOR)
            .map(|_| SHADER_ATTRIB_COLOR_LEN).unwrap_or(0) +
        pipeline.reflection.get_attr_loc(SHADER_ATTRIB_TEXCOORD)
            .map(|_| SHADER_ATTRIB_TEXCOORD_LEN).unwrap_or(0);

    let mut total_vertices = 0;

    #[allow(unused_assignments)]
    {
        let staging_buf = proc_obj.staging_buffer.as_mut().unwrap();
        let mut staging_buf_map =
            staging_buf.map(device, 0, vk::WHOLE_SIZE, vk::MemoryMapFlags::empty()).unwrap();
        let float_buffer = staging_buf_map.as_slice_mut();
        for prim in object.get_primitives() {
            for vertex in prim.get_vertices() {
                let major_off = (total_vertices * vertex_comps) as usize;
                let mut minor_off = 0usize;

                let pos_vec = Vector4f::new(vertex.position.x, vertex.position.y, 0.0, 1.0);
                let transformed_pos = transform * pos_vec;
                float_buffer[major_off + minor_off] = transformed_pos.x;
                float_buffer[major_off + minor_off + 1] = transformed_pos.y;
                minor_off += 2;

                total_vertices += 1;
            }
        }
    }

    proc_obj.visited = true;
    proc_obj.updated = true;
}
