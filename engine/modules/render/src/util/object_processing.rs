use std::collections::HashMap;
use std::sync::atomic::Ordering;
use argus_util::math::Vector2f;
use argus_util::pool::Handle;
use crate::common::{Matrix4x4, SceneItemType};
use crate::twod::{get_render_context_2d, RenderContext2d};

pub fn process_objects_2d<S>(
    scene_id: impl AsRef<str>,
    update_fn: fn(&str, Handle, &Matrix4x4, bool, &mut S),
    state: &mut S,
) {
    let mut new_version_map: HashMap<(SceneItemType, Handle), u16> = HashMap::new();

    let root_group_handle = {
        let scene = get_render_context_2d().get_scene(scene_id.as_ref()).unwrap();
        scene.root_group_write.unwrap()
    };
    process_render_group_2d(
        scene_id.as_ref(),
        root_group_handle, //TODO
        false,
        Matrix4x4::identity(),
        &mut new_version_map,
        update_fn,
        state,
    );

    let mut scene = get_render_context_2d().get_scene_mut(scene_id.as_ref()).unwrap();
    // this map is internal to the renderer and thus doesn't need to be synchronized
    scene.last_rendered_versions = new_version_map;
}

fn compute_abs_group_transform(context: &RenderContext2d, group: Handle)
    -> Matrix4x4 {
    let mut mat: Matrix4x4;
    let mut cur_handle_opt: Option<Handle>;
    {
        let mut initial_group = context.get_group_mut(group).unwrap();
        mat = initial_group.get_transform().value.as_matrix(Vector2f::new(0.0, 0.0));
        cur_handle_opt = initial_group.get_parent();
    }

    while let Some(cur_handle) = cur_handle_opt {
        let cur_group = context.get_group_mut(cur_handle).unwrap();
        cur_handle_opt = cur_group.get_parent();
        mat = mat * cur_group.peek_transform().as_matrix(Vector2f::new(0.0, 0.0));
    }

    mat
}

fn process_render_group_2d<S>(
    scene_id: &str,
    group_handle: Handle,
    recompute_transform: bool,
    running_transform: Matrix4x4,
    new_version_map: &mut HashMap<(SceneItemType, Handle), u16>,
    update_fn: fn(&str, Handle, &Matrix4x4, bool, &mut S),
    state: &mut S,
) {
    let context = get_render_context_2d();

    let (group_version, child_groups, child_objects, group_transform) = {
        let mut group = context.get_group_mut(group_handle).unwrap();
        (
            group.version.load(Ordering::Relaxed),
            group.child_groups.clone(),
            group.child_objects.clone(),
            group.get_transform(),
        )
    };

    let cur_version_map = {
        let scene = context.get_scene_mut(scene_id).unwrap();
        scene.last_rendered_versions.clone()
    };

    new_version_map.insert((SceneItemType::Group, group_handle), group_version);

    let mut cur_transform = Matrix4x4::identity();
    let mut recompute_child_transform = recompute_transform;
    if recompute_transform {
        // we already know we have to recompute the transform of this whole
        // branch since a parent was dirty
        cur_transform = group_transform.value.as_matrix(Vector2f::new(0.0, 0.0)) *
            running_transform;
    } else if group_version !=
        *cur_version_map.get(&(SceneItemType::Group, group_handle)).unwrap_or(&0) {
        cur_transform = compute_abs_group_transform(context, group_handle);
        recompute_child_transform = true;
    }

    for child_obj_handle in child_objects {
        let (obj_transform, obj_anchor, child_version) = {
            //TODO: stopgap until render graph buffering is properly implemented
            let Some(mut child_object) = get_render_context_2d()
                .get_object_mut(child_obj_handle)
                else { continue; };

            (
                child_object.get_transform().value,
                child_object.get_anchor_point(),
                child_object.version.load(Ordering::Relaxed),
            )
        };

        let is_obj_dirty = child_version !=
            *cur_version_map.get(&(SceneItemType::Object, child_obj_handle)).unwrap_or(&0);

        new_version_map.insert((SceneItemType::Object, child_obj_handle), child_version);

        let final_obj_transform = if recompute_child_transform {
            cur_transform * obj_transform.as_matrix(obj_anchor)
        } else if is_obj_dirty {
            // parent transform hasn't been computed so we need to do it here
            compute_abs_group_transform(get_render_context_2d(), group_handle) *
                obj_transform.as_matrix(obj_anchor)
        } else {
            Matrix4x4::identity()
        };
        // don't need to compute anything otherwise, update function will just
        // mark the object as visited

        let dirty_transform = recompute_child_transform || is_obj_dirty;

        update_fn(
            scene_id.as_ref(),
            child_obj_handle,
            &final_obj_transform,
            dirty_transform,
            state,
        );
    }

    for child_group_handle in child_groups {
        process_render_group_2d(
            scene_id,
            child_group_handle,
            recompute_child_transform,
            cur_transform,
            new_version_map,
            update_fn,
            state,
        );
    }
}
