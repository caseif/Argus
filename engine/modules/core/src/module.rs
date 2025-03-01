use std::collections::{HashMap, VecDeque};
use num_enum::{IntoPrimitive, TryFromPrimitive};
use argus_scripting_bind::script_bind;
use crate::EngineError;
use crate::internal::register::{ModuleRegistration, REGISTERED_MODULES};

#[derive(
    Clone, Copy, Debug, Eq, Hash, IntoPrimitive,
    Ord, PartialEq, PartialOrd, TryFromPrimitive,
)]
#[repr(u32)]
#[script_bind]
pub enum LifecycleStage {
    Load,
    PreInit,
    Init,
    PostInit,
    Running,
    PreDeinit,
    Deinit,
    PostDeinit,
}

// dynamic modules aren't supported post-Rust rewrite, at least for now

pub fn register_dynamic_module(
    _id: &str,
    _lifecycle_callback: fn(LifecycleStage),
    _dependencies: Vec<&str>,
) {
    panic!("Dynamic modules are not currently supported");
}

pub fn enable_dynamic_module(_module_id: &str) -> bool {
    false
}

pub fn get_present_dynamic_modules() -> Vec<String> {
    Vec::new()
}

pub fn get_present_static_modules() -> Vec<String> {
    REGISTERED_MODULES.iter().map(|reg| reg.id.to_owned()).collect()
}

pub(crate) fn get_registered_modules_sorted()
    -> Result<Vec<&'static ModuleRegistration>, EngineError> {
    let all_modules: HashMap<&'static str, &'static ModuleRegistration> =
        REGISTERED_MODULES.iter().map(|reg| (reg.id, reg)).collect();

    let mut edges: Vec<(&str, &str)> = Vec::new();

    for reg in REGISTERED_MODULES {
        for &dep_id in reg.depends_on {
            if !all_modules.contains_key(dep_id) {
                return Err(EngineError::new(format!(
                    "Module with ID '{}' (declared as dependency of '{}') is not registered!",
                    dep_id,
                    reg.id,
                )));
            }

            edges.push((dep_id, reg.id))
        }
    }

    topo_sort(&all_modules, edges)
}

fn topo_sort<'a>(
    nodes: &HashMap<&str, &'a ModuleRegistration>,
    edges: Vec<(&str, &str)>
) -> Result<Vec<&'a ModuleRegistration>, EngineError> {
    let mut sorted_nodes: Vec<&'a ModuleRegistration> = Vec::new();
    let mut start_nodes: VecDeque<&str> = VecDeque::new();
    let mut remaining_edges: Vec<(&str, &str)> = edges;

    for (&id, _) in nodes {
        start_nodes.push_back(id);
    }

    for &(_, dest) in &remaining_edges {
        start_nodes.retain(|&node| node != dest);
    }

    while let Some(cur_node_id) = start_nodes.pop_front() {
        let cur_node = nodes[cur_node_id];
        if !sorted_nodes.contains(&cur_node) {
            sorted_nodes.push(cur_node);
        }

        let mut remove_edges: Vec<(&str, &str)> = Vec::new();
        for cur_edge in &remaining_edges {
            let &(src_node, dest_node) = cur_edge;
            if src_node != cur_node_id {
                continue;
            }

            remove_edges.push(*cur_edge);

            let mut has_incoming_edges = false;
            for check_edge in &remaining_edges {
                let &(_, check_dest) = check_edge;
                if check_edge != cur_edge && check_dest == dest_node {
                    has_incoming_edges = true;
                    break;
                }
            }

            if !has_incoming_edges {
                start_nodes.push_back(dest_node);
            }
        }

        for edge in &remove_edges {
            remaining_edges.retain(|item| item != edge);
        }
    }

    if !remaining_edges.is_empty() {
        return Err(EngineError::new("Graph contains cycles"));
    }

    Ok(sorted_nodes)
}
