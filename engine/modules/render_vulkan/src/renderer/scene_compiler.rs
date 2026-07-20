use std::sync::atomic::Ordering;
use argus_render::util::process_objects_2d;
use vk_wrapper::vk::MAX_FRAMES_IN_FLIGHT;
use crate::renderer::object_proc_impl::process_object;
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
    state: &mut RendererState<'_>,
    scene_id: &str,
) {
    process_objects_2d(
        scene_id,
        process_object,
        state,
    );
    
    let scene_state = state.scene_states_2d.get_mut(scene_id).unwrap();

    // objects not visited during the last iteration
    // must not be present in the scene graph anymore
    let stale_objs = scene_state.processed_objs
        .extract_if(|_, obj| !obj.newly_created && !obj.visited)
        .collect::<Vec<_>>();
    if !stale_objs.is_empty() {
        let last_frame = (state.cur_frame.load(Ordering::Relaxed) + MAX_FRAMES_IN_FLIGHT - 1) %
            MAX_FRAMES_IN_FLIGHT;
        let last_frame_fence = &state.swapchain.as_ref().unwrap().in_flight_fence[last_frame];
        state.device.wait_for_fences(&[last_frame_fence], true, u64::MAX).unwrap();

        for (stale_obj_handle, stale_obj) in stale_objs {
            let bucket = scene_state.render_buckets.get_mut(&get_bucket_key(&stale_obj)).unwrap();
            bucket.objects.retain(|h| h != &stale_obj_handle);
            bucket.needs_rebuild = true;
            stale_obj.destroy();
        }
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
