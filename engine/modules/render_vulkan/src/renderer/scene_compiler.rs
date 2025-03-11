use argus_render::util::process_objects_2d;
use crate::renderer::object_proc_impl::process_object;
use crate::setup::device::VulkanDevice;
use crate::setup::instance::VulkanInstance;
use crate::state::{ProcessedObject, RenderBucket, RenderBucketKey, RendererState};

fn get_bucket_key(processed_obj: &ProcessedObject) -> RenderBucketKey {
    RenderBucketKey::new(
        processed_obj.material_res.get_prototype().uid.clone(),
        &processed_obj.atlas_stride,
        processed_obj.z_index,
        processed_obj.light_opacity,
    )
}

pub(crate) fn compile_scene_2d(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    state: &mut RendererState,
    scene_id: &str,
) {
    process_objects_2d(
        scene_id,
        process_object,
        &mut (instance, device, state),
    );
    
    let scene_state = state.scene_states_2d.get_mut(scene_id).unwrap();

    // objects not visited during the last iteration
    // must not be present in the scene graph anymore
    for (stale_obj_handle, stale_obj) in scene_state.processed_objs
        .extract_if(|_, obj| !obj.newly_created && !obj.visited) {
        let bucket = scene_state.render_buckets.get_mut(&get_bucket_key(&stale_obj)).unwrap();
        bucket.objects.retain(|h| h != &stale_obj_handle);
        bucket.needs_rebuild = true;
        stale_obj.destroy(device);
    }

    for (obj_handle, obj) in &mut scene_state.processed_objs {
        if obj.newly_created {
            let key = get_bucket_key(obj);
            let bucket = scene_state.render_buckets.entry(key).or_insert_with(|| {
                RenderBucket::new(
                    obj.material_res.clone(),
                    obj.atlas_stride,
                    obj.z_index,
                    obj.light_opacity,
                )
            });

            bucket.objects.push(*obj_handle);
            bucket.needs_rebuild = true;

            obj.newly_created = false;
        }

        // reset visited flag for next iteration
        obj.visited = false;
    }
}
