/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

use std::collections::HashMap;
use std::convert::TryInto;
use std::fmt;
use std::fmt::Write;

use glsl::{parser::*, syntax::*, visitor::*};
use glslang::*;

const LAYOUT_ID_LOCATION: &str = "location";
const LAYOUT_ID_BINDING: &str = "binding";
const LAYOUT_ID_STD140: &str = "std140";
const LAYOUT_ID_STD430: &str = "std430";

pub struct ProcessedGlslShader {
    stage: glslang::ShaderStage,
    source: String,
    inputs: HashMap<String, u32>,
    outputs: HashMap<String, u32>,
    uniforms: HashMap<String, u32>,
    buffers: HashMap<String, u32>,
    ubo_bindings: HashMap<String, u32>,
    ubo_names: HashMap<String, String>,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct GlslCompileError {
    pub message: String,
}

impl GlslCompileError {
    fn new(message: impl Into<String>) -> Self {
        GlslCompileError {
            message: message.into(),
        }
    }
}

impl std::error::Error for GlslCompileError {}

impl fmt::Display for GlslCompileError {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "GLSL compilation failed: {}", self.message)
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct InvalidArgumentError {
    pub argument: String,
    pub message: String,
}

impl InvalidArgumentError {
    fn new(argument: &str, message: &str) -> Self {
        InvalidArgumentError {
            argument: argument.to_string(),
            message: message.to_string(),
        }
    }
}

impl std::error::Error for InvalidArgumentError {}

impl fmt::Display for InvalidArgumentError {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(
            f,
            "Invalid value passed for argument {} ({})",
            self.argument, self.message
        )
    }
}

struct StringWriter {
    string: String,
}

impl StringWriter {
    fn new() -> Self {
        StringWriter {
            string: "".to_string(),
        }
    }

    fn to_string(&self) -> String {
        self.string.clone()
    }
}

impl Write for StringWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.string.push_str(s);
        Ok(())
    }
}

#[derive(Debug)]
pub struct CompiledShaderSet {
    pub bytecode: HashMap<glslang::ShaderStage, Vec<u32>>,
    pub inputs: HashMap<String, u32>,
    pub outputs: HashMap<String, u32>,
    pub uniforms: HashMap<String, u32>,
    pub buffers: HashMap<String, u32>,
    pub ubo_bindings: HashMap<String, u32>,
    pub ubo_names: HashMap<String, String>,
}

pub fn compile_glsl_to_spirv(
    glsl_sources: &HashMap<glslang::ShaderStage, String>,
    client: Target,
    client_version: i32,
) -> Result<CompiledShaderSet, GlslCompileError> {
    let mut res = HashMap::<glslang::ShaderStage, Vec<u32>>::with_capacity(glsl_sources.len());

    let compiler = Compiler::acquire().unwrap();

    let processed_glsl = process_glsl(glsl_sources)?;

    let options = CompilerOptions {
        source_language: SourceLanguage::GLSL,
        target: client,
        version_profile: Some((client_version, GlslProfile::Core)),
        messages: ShaderMessage::empty(),
    };

    for glsl in &processed_glsl {
        let source = ShaderSource::from(glsl.source.as_str());

        let input = ShaderInput::new(
            &source,
            glsl.stage,
            &options,
            None,
            None,
        ).unwrap();

        let shader = match Shader::new(compiler, input) {
            Ok(shader) => shader,
            Err(err) => {
                return Err(GlslCompileError::new(err.to_string()));
            },
        };

        let mut program = Program::new(compiler);
        program.add_shader(&shader);

        let code = program.compile(glsl.stage)
            .expect("Failed to compile GLSL shader");
        res.insert(glsl.stage, code);
    }

    let program_attrs = processed_glsl
        .iter()
        .find(|glsl| glsl.stage == glslang::ShaderStage::Vertex)
        .map(|glsl| glsl.inputs.clone())
        .unwrap_or(HashMap::new());

    let program_outputs = processed_glsl
        .iter()
        .find(|glsl| glsl.stage == glslang::ShaderStage::Fragment)
        .map(|glsl| glsl.outputs.clone())
        .unwrap_or(HashMap::new());

    let program_uniforms =
        processed_glsl
            .iter()
            .map(|glsl| &glsl.uniforms)
            .fold(HashMap::new(), |mut a, e| {
                a.extend(e.clone());
                a
            });

    let program_buffers =
        processed_glsl
            .iter()
            .map(|glsl| &glsl.buffers)
            .fold(HashMap::new(), |mut a, e| {
                a.extend(e.clone());
                a
            });

    let program_ubo_bindings =
        processed_glsl
            .iter()
            .map(|glsl| &glsl.ubo_bindings)
            .fold(HashMap::new(), |mut a, e| {
                a.extend(e.clone());
                a
            });

    let program_ubo_names =
        processed_glsl
            .iter()
            .map(|glsl| &glsl.ubo_names)
            .fold(HashMap::new(), |mut a, e| {
                a.extend(e.clone());
                a
            });

    Ok(CompiledShaderSet {
        bytecode: res,
        inputs: program_attrs,
        outputs: program_outputs,
        uniforms: program_uniforms,
        buffers: program_buffers,
        ubo_bindings: program_ubo_bindings,
        ubo_names: program_ubo_names,
    })
}

fn get_free_loc_ranges(existing_decls: Vec<DeclarationInfo>) -> (Vec<(u32, u32)>, u32) {
    // handle pathological case specially
    if existing_decls.is_empty() {
        return (Vec::new(), 0);
    }

    let mut occupied_locs = Vec::<bool>::new();

    for decl in existing_decls {
        assert!(decl.size.is_some());

        if decl.location.is_none() {
            continue;
        }

        let range_end = decl.location.unwrap() + decl.size.unwrap();
        if range_end > occupied_locs.len().try_into().unwrap() {
            occupied_locs.resize(range_end as usize, false);
        }

        for i in 0..decl.size.unwrap() {
            occupied_locs[(decl.location.unwrap() + i) as usize] = true;
        }
    }

    assert!(occupied_locs.len() <= u32::MAX as usize);

    let mut free_ranges = Vec::<(u32, u32)>::new(); // (pos, size)

    let mut range_start = 0;
    for (i, &occupied) in occupied_locs.iter().enumerate() {
        if occupied {
            if range_start != i {
                free_ranges.push((
                    range_start.try_into().unwrap(),
                    (i - range_start).try_into().unwrap(),
                ));
            }
            range_start = i + 1;
        }
    }

    (free_ranges, occupied_locs.len().try_into().unwrap())
}

fn get_next_free_location(size: u32, free_range_info: &mut (Vec<(u32, u32)>, u32)) -> u32 {
    let free_ranges = &mut free_range_info.0;
    let max_occupied_plus_one = &mut free_range_info.1;

    for i in 0..free_ranges.len() {
        let range = free_ranges[i];
        if range.1 >= size {
            let loc = range.0;

            let new_size = range.1 - size;
            if new_size > 0 {
                free_ranges[i] = (range.0 + size, new_size);
            } else {
                free_ranges.remove(i);
            }

            return loc;
        }
    }

    let loc = *max_occupied_plus_one;
    *max_occupied_plus_one += size;
    loc
}

fn process_glsl(
    glsl_sources: &HashMap<glslang::ShaderStage, String>,
) -> Result<Vec<ProcessedGlslShader>, GlslCompileError> {
    let mut processed_sources = Vec::<ProcessedGlslShader>::with_capacity(glsl_sources.len());

    let mut sorted_sources: Vec<(glslang::ShaderStage, String)> = glsl_sources
        .iter()
        .map(|kv| (kv.0.to_owned(), kv.1.to_owned()))
        .collect();
    sorted_sources.sort_by_key(|entry| entry.0 as u32);

    let mut struct_sizes = HashMap::<glslang::ShaderStage, HashMap<String, Option<u32>>>::new();

    let mut pending_asts = HashMap::<glslang::ShaderStage, TranslationUnit>::new();

    let mut inputs = HashMap::<glslang::ShaderStage, HashMap<String, DeclarationInfo>>::new();
    let mut outputs = HashMap::<glslang::ShaderStage, HashMap<String, DeclarationInfo>>::new();
    let mut uniforms = HashMap::<String, DeclarationInfo>::new();
    let mut buffers = HashMap::<glslang::ShaderStage, HashMap<String, DeclarationInfo>>::new();
    let mut ubos = HashMap::<String, BlockInfo>::new();

    // first pass: enumerate declarations of all shaders
    let mut i = 0;
    for kv in &sorted_sources {
        let stage = kv.0;
        let source = &kv.1;

        let ast_res = glsl::syntax::ShaderStage::parse(source);
        if ast_res.is_err() {
            return Err(GlslCompileError {
                message: format!("Failed to parse GLSL: {}", ast_res.unwrap_err().info),
            });
        }

        let ast = ast_res.unwrap();

        let mut struct_visitor = StructDefVisitor::new();
        ast.visit(&mut struct_visitor);

        if struct_visitor.fail_condition {
            return Err(GlslCompileError {
                message: struct_visitor.fail_message.unwrap(),
            });
        }

        let mut scan_visitor = ScanningDeclarationVisitor::new(&struct_visitor.struct_sizes);
        ast.visit(&mut scan_visitor);

        if scan_visitor.fail_condition {
            return Err(GlslCompileError {
                message: scan_visitor.fail_message.unwrap(),
            });
        }

        // validate uniforms to ensure we don't have conflicting definitions
        for (uni_name, cur_decl) in scan_visitor.uniforms {
            if let std::collections::hash_map::Entry::Vacant(e) = uniforms.entry(uni_name.clone()) {
                e.insert(cur_decl);
            } else {
                let prev_decl = uniforms.get_mut(&uni_name).unwrap();

                if cur_decl.explicit_location {
                    if prev_decl.explicit_location && prev_decl.location != cur_decl.location {
                        return Err(GlslCompileError {
                            message: "Shader set contains conflicting location qualifiers for \
                                same uniform name".to_string()
                        });
                    }

                    prev_decl.explicit_location = true;
                    prev_decl.location = cur_decl.location;
                }

                if prev_decl.size != cur_decl.size {
                    return Err(GlslCompileError {
                        message: "Shader set contains conflicting sizes for same uniform name"
                            .to_string(),
                    });
                }
            }
        }

        // validate blocks to ensure we don't have conflicting definitions
        for (block_name, cur_decl) in scan_visitor.ubos {
            if let std::collections::hash_map::Entry::Vacant(e) = ubos.entry(block_name.clone()) {
                e.insert(cur_decl);
            } else {
                let prev_decl = ubos.get_mut(&block_name).unwrap();

                if prev_decl.storage != cur_decl.storage {
                    return Err(GlslCompileError::new(
                        "Shader set contains conflicting storage qualifiers for same uniform name",
                    ));
                }

                if prev_decl.instance_name != cur_decl.instance_name {
                    return Err(GlslCompileError::new(
                        "All shader stages must use same instance name per UBO",
                    ));
                }
            }
        }

        inputs.insert(stage, scan_visitor.inputs);
        outputs.insert(stage, scan_visitor.outputs);
        buffers.insert(stage, scan_visitor.buffers);

        pending_asts.insert(stage, ast);

        struct_sizes.insert(stage, struct_visitor.struct_sizes);

        i += 1;
    }

    let mut all_assigned_inputs = HashMap::<glslang::ShaderStage, HashMap<String, DeclarationInfo>>::new();
    let mut all_assigned_outputs = HashMap::<glslang::ShaderStage, HashMap<String, DeclarationInfo>>::new();
    let mut all_assigned_buffers = HashMap::<glslang::ShaderStage, HashMap<String, DeclarationInfo>>::new();

    i = 0;
    for stage in sorted_sources.iter().map(|kv| kv.0) {
        let prev_stage_opt = if i > 0 {
            Some(sorted_sources[i - 1].0)
        } else {
            None
        };
        let next_stage_opt = if i < sorted_sources.len() - 1 {
            Some(sorted_sources[i + 1].0)
        } else {
            None
        };

        {
            // handle inputs

            let mut occupied_input_locs = Vec::<DeclarationInfo>::new();

            for in_decl in inputs.get_mut(&stage).unwrap().values_mut() {
                // skip inputs with explicit locations
                if in_decl.location.is_some() {
                    occupied_input_locs.push(in_decl.clone());
                    continue;
                }

                if let Some(prev_stage) = prev_stage_opt {
                    // assign locations for inputs with matching outputs from previous stage
                    if let Some(matching_out) = outputs[&prev_stage].get(&in_decl.name) {
                        in_decl.location = matching_out.location;
                        occupied_input_locs.push(in_decl.clone());

                        continue;
                    }
                }
            }

            if let Some(prev_stage) = prev_stage_opt {
                for out_decl in outputs.get_mut(&prev_stage).unwrap().values() {
                    if out_decl.location.is_some() {
                        occupied_input_locs.push(out_decl.clone());
                    }
                }
            }

            let mut avail_input_locs = get_free_loc_ranges(occupied_input_locs);

            for in_decl in inputs.get_mut(&stage).unwrap().values_mut() {
                // skip inputs that already have assigned locations
                if in_decl.location.is_some() {
                    continue;
                }

                // otherwise, just pick the next available location
                assert!(in_decl.size.is_some());
                in_decl.location = Some(get_next_free_location(
                    in_decl.size.unwrap(),
                    &mut avail_input_locs,
                ));
            }
        }

        {
            // handle outputs

            let mut occupied_output_locs = Vec::<DeclarationInfo>::new();

            if let Some(next_stage) = next_stage_opt {
                for in_decl in inputs.get_mut(&next_stage).unwrap().values() {
                    if in_decl.location.is_some() {
                        occupied_output_locs.push(in_decl.clone());
                    }
                }
            }

            let mut avail_output_locs = get_free_loc_ranges(occupied_output_locs);

            for out_decl in outputs.get_mut(&stage).unwrap() {
                // skip outputs that already have assigned locations
                if out_decl.1.location.is_some() {
                    continue;
                }

                // otherwise, just pick the next available location
                assert!(out_decl.1.size.is_some());
                out_decl.1.location = Some(get_next_free_location(
                    out_decl.1.size.unwrap(),
                    &mut avail_output_locs,
                ));
            }
        }

        all_assigned_inputs.insert(stage, inputs[&stage].clone());
        all_assigned_outputs.insert(stage, outputs[&stage].clone());
        all_assigned_buffers.insert(stage, buffers[&stage].clone());

        i += 1;
    }

    // handle uniforms

    let occupied_uniform_locs = uniforms
        .values()
        .filter(|decl| decl.location.is_some())
        .cloned()
        .collect();
    let mut avail_uniform_locs = get_free_loc_ranges(occupied_uniform_locs);

    for uni_decl in uniforms.values_mut() {
        // skip uniforms that already have assigned locations or are opaque
        if uni_decl.location.is_some() || uni_decl.size.unwrap_or(0) == 0 {
            continue;
        }

        // otherwise, just pick the next available location
        assert!(uni_decl.size.is_some());
        uni_decl.location = Some(get_next_free_location(
            uni_decl.size.unwrap(),
            &mut avail_uniform_locs,
        ));
    }

    // second pass: apply assigned locations to the AST
    for ast_kv in &mut pending_asts {
        let stage = ast_kv.0;
        let ast = ast_kv.1;

        // first, change the version to the latest GLSL revision
        let mut pp_version_visitor = PreprocessorVersionVisitor::new();
        ast.visit_mut(&mut pp_version_visitor);

        // second pass: set locations for all input declarations which match an
        // output of the previous stage by name, and record all declarations
        // which still need to be assigned locations

        let mut mutate_visitor = MutatingDeclarationVisitor::new(
            &struct_sizes[stage],
            &all_assigned_inputs[stage],
            &all_assigned_outputs[stage],
            &uniforms,
            &all_assigned_buffers[stage],
        );
        ast.visit_mut(&mut mutate_visitor);

        if mutate_visitor.fail_condition {
            return Err(GlslCompileError {
                message: mutate_visitor.fail_message.unwrap(),
            });
        }

        // finally, transpile the transformed shader back to GLSL

        let mut src_writer = StringWriter::new();
        glsl::transpiler::glsl::show_translation_unit(&mut src_writer, ast);

        let processed_src = src_writer.to_string();

        processed_sources.push(ProcessedGlslShader {
            stage: stage.to_owned(),
            source: processed_src,
            inputs: all_assigned_inputs[stage]
                .iter()
                .filter(|(_, v)| v.location.is_some())
                .map(|(k, v)| (k.clone(), v.location.unwrap()))
                .collect(),
            outputs: all_assigned_outputs[stage]
                .iter()
                .filter(|(_, v)| v.location.is_some())
                .map(|(k, v)| (k.clone(), v.location.unwrap()))
                .collect(),
            uniforms: uniforms
                .iter()
                .filter(|(_, v)| v.location.is_some())
                .map(|(k, v)| (k.clone(), v.location.unwrap()))
                .collect(),
            buffers: all_assigned_buffers[stage]
                .iter()
                .filter(|(_, v)| v.location.is_some())
                .map(|(k, v)| (k.clone(), v.location.unwrap()))
                .collect(),
            ubo_bindings: ubos
                .iter()
                .filter(|(_, v)| v.binding.is_some())
                .map(|(k, v)| (k.clone(), v.binding.unwrap()))
                .collect(),
            ubo_names: ubos
                .iter()
                .filter(|(_, v)| v.binding.is_some())
                .map(|(k, v)| (k.clone(), v.instance_name.to_owned()))
                .collect(),
        });
    }

    Ok(processed_sources)
}

fn get_array_size_multiplier(
    arr_spec_opt: &Option<ArraySpecifier>,
) -> Result<u32, GlslCompileError> {
    match &arr_spec_opt {
        Some(arr_spec) => {
            let mut total_size = 1u32;
            for dim in &arr_spec.dimensions {
                total_size *= match dim {
                    ArraySpecifierDimension::ExplicitlySized(size_expr) => {
                        match size_expr.as_ref() {
                            Expr::IntConst(size) => {
                                if *size > 0 {
                                    *size as u32
                                } else {
                                    return Err(GlslCompileError::new(
                                        "Array dimension specifier must be positive \
                                        non-zero integer",
                                    ));
                                }
                            }
                            _ => {
                                return Err(GlslCompileError::new(
                                    "Non-literal array dimension specifiers are not \
                                    supported at this time",
                                ));
                            }
                        }
                    }
                    ArraySpecifierDimension::Unsized => {
                        return Err(GlslCompileError::new(
                            "Array specifier for type must be explicitly sized",
                        ));
                    }
                };
            }

            Ok(total_size)
        }
        None => Ok(1u32),
    }
}

fn get_type_size(type_spec: &TypeSpecifierNonArray) -> Result<u32, InvalidArgumentError> {
    match type_spec {
        TypeSpecifierNonArray::Void
        | TypeSpecifierNonArray::Bool
        | TypeSpecifierNonArray::Int
        | TypeSpecifierNonArray::UInt
        | TypeSpecifierNonArray::Float
        | TypeSpecifierNonArray::Double
        | TypeSpecifierNonArray::Vec2
        | TypeSpecifierNonArray::Vec3
        | TypeSpecifierNonArray::Vec4
        | TypeSpecifierNonArray::DVec2
        | TypeSpecifierNonArray::BVec2
        | TypeSpecifierNonArray::BVec3
        | TypeSpecifierNonArray::BVec4
        | TypeSpecifierNonArray::IVec2
        | TypeSpecifierNonArray::IVec3
        | TypeSpecifierNonArray::IVec4
        | TypeSpecifierNonArray::UVec2
        | TypeSpecifierNonArray::UVec3
        | TypeSpecifierNonArray::UVec4 => Ok(1),
        TypeSpecifierNonArray::Mat2
        | TypeSpecifierNonArray::Mat23
        | TypeSpecifierNonArray::Mat24 => Ok(2),
        TypeSpecifierNonArray::Mat3
        | TypeSpecifierNonArray::Mat32
        | TypeSpecifierNonArray::Mat34 => Ok(3),
        TypeSpecifierNonArray::Mat4
        | TypeSpecifierNonArray::Mat42
        | TypeSpecifierNonArray::Mat43 => Ok(4),
        TypeSpecifierNonArray::DMat2
        | TypeSpecifierNonArray::DMat23
        | TypeSpecifierNonArray::DMat24 => Ok(4),
        TypeSpecifierNonArray::DMat3
        | TypeSpecifierNonArray::DMat32
        | TypeSpecifierNonArray::DMat34 => Ok(6),
        TypeSpecifierNonArray::DMat4
        | TypeSpecifierNonArray::DMat42
        | TypeSpecifierNonArray::DMat43 => Ok(8),
        TypeSpecifierNonArray::DVec3 | TypeSpecifierNonArray::DVec4 => Ok(2),
        TypeSpecifierNonArray::TypeName(_) => Err(InvalidArgumentError::new(
            "type_spec",
            "Cannot get size of non-builtin type",
        )),
        _ => Ok(0),
    }
}

struct StructDefVisitor {
    fail_condition: bool,
    fail_message: Option<String>,
    struct_sizes: HashMap<String, Option<u32>>,
}

impl StructDefVisitor {
    fn new() -> Self {
        StructDefVisitor {
            fail_condition: false,
            fail_message: None,
            struct_sizes: HashMap::new(),
        }
    }
}

impl Visitor for StructDefVisitor {
    fn visit_struct_specifier(&mut self, struct_spec: &StructSpecifier) -> Visit {
        if self.fail_condition {
            return Visit::Parent;
        }

        if struct_spec.name.is_none() {
            return Visit::Parent;
        }

        let mut total_size = 0u32;

        for sf in &struct_spec.fields {
            let ts = &sf.ty;
            let base_size = if let TypeSpecifierNonArray::TypeName(tn) = &ts.ty {
                let size_opt = self.struct_sizes.get(&tn.0);
                if let Some(size) = size_opt {
                    size.unwrap_or(0)
                } else {
                    self.fail_condition = true;
                    self.fail_message =
                        Some(format!("Unknown type name {} referenced in struct", tn.0));
                    0u32
                }
            } else {
                get_type_size(&ts.ty).unwrap()
            };

            if self.fail_condition {
                return Visit::Parent;
            }

            let arr_mult = match base_size {
                0 => 0,
                _ => {
                    let arr_mult_res = get_array_size_multiplier(&ts.array_specifier);
                    if arr_mult_res.is_err() {
                        self.fail_condition = true;
                        self.fail_message = Some(arr_mult_res.unwrap_err().message);
                        return Visit::Parent;
                    }

                    arr_mult_res.unwrap()
                }
            };

            if base_size == 0 {
                // struct is opaque, just flag it as such and skip the rest of the fields
                self.struct_sizes
                    .insert(struct_spec.name.as_ref().unwrap().0.to_owned(), None);
                return Visit::Parent;
            }

            total_size += base_size * arr_mult;
        }

        self.struct_sizes.insert(
            struct_spec.name.as_ref().unwrap().0.to_owned(),
            Some(total_size),
        );

        Visit::Parent
    }
}

#[derive(Clone)]
struct DeclarationInfo {
    name: String,
    storage: StorageQualifier,
    size: Option<u32>,
    location: Option<u32>,
    explicit_location: bool,
}

#[derive(Clone)]
struct BlockInfo {
    block_name: String,
    instance_name: String,
    #[allow(dead_code)]
    array_dims: Option<u32>,
    storage: StorageQualifier,
    binding: Option<u32>,
}

fn get_decl_info(
    single_decl: &SingleDeclaration,
    struct_sizes: &HashMap<String, Option<u32>>,
) -> Result<Option<DeclarationInfo>, GlslCompileError> {
    let name = &single_decl.name;
    if name.is_none() {
        return Ok(None);
    }

    let decl_type = &single_decl.ty;

    let qualifiers = &decl_type.qualifier;
    if qualifiers.is_none() {
        // not interested if it's not in/out/uniform/buffer
        return Ok(None);
    }

    let mut location: Option<u32> = None;

    let mut storage: Option<StorageQualifier> = None;

    for qualifier in &qualifiers.as_ref().unwrap().qualifiers {
        match qualifier {
            TypeQualifierSpec::Storage(StorageQualifier::Attribute) => {
                return Err(GlslCompileError::new(
                    "Storage qualifier 'attribute' is not supported",
                ));
            }
            TypeQualifierSpec::Storage(StorageQualifier::Varying) => {
                return Err(GlslCompileError::new(
                    "Storage qualifier 'varying' is not supported",
                ));
            }
            TypeQualifierSpec::Storage(qual) => {
                storage = Some(qual.to_owned());
            }
            TypeQualifierSpec::Layout(layout) => {
                for id_enum in &layout.ids {
                    match id_enum {
                        LayoutQualifierSpec::Identifier(id, expr) => {
                            if id.to_string() == LAYOUT_ID_LOCATION {
                                if expr.is_none() {
                                    return Err(GlslCompileError::new(
                                        "Location layout qualifier requires assignment",
                                    ));
                                }
                                match expr.as_ref().unwrap().as_ref() {
                                    Expr::IntConst(loc) => {
                                        if loc >= &0 {
                                            location = Some(loc.to_owned() as u32);
                                        } else {
                                            return Err(GlslCompileError::new(
                                                "Location layout qualifier must not be negative",
                                            ));
                                        }
                                    }
                                    Expr::UIntConst(_)
                                    | Expr::FloatConst(_)
                                    | Expr::DoubleConst(_)
                                    | Expr::BoolConst(_) => {
                                        return Err(GlslCompileError::new(
                                            "Location layout \
                                            qualifier requires scalar integer expression",
                                        ));
                                    }
                                    _ => {
                                        return Err(GlslCompileError::new(
                                            "Non-literal layout \
                                            location values are not supported at this time",
                                        ));
                                    }
                                }
                            }
                        }
                        LayoutQualifierSpec::Shared => (),
                    };
                }
            }
            _ => (),
        }
    }

    if storage.is_none() {
        return Ok(None);
    }

    let type_size = if let TypeSpecifierNonArray::TypeName(tn) = &decl_type.ty.ty {
        let size = struct_sizes.get(&tn.0);
        if size.is_some() {
            match size.unwrap().unwrap_or(0) {
                0 => None,
                v => Some(v),
            }
        } else {
            return Err(GlslCompileError::new(
                std::format!(
                    "Unknown type name {} \
                referenced in declaration '{}'",
                    &tn.0,
                    name.as_ref().unwrap().0
                )
                .as_str(),
            ));
        }
    } else {
        Some(get_type_size(&decl_type.ty.ty).unwrap())
    };

    let arr_el_count = &single_decl
        .array_specifier
        .as_ref()
        .map(|arr_spec| {
            arr_spec.dimensions.0.iter().try_fold(1, |a, dim| {
                let mult = match dim {
                    ArraySpecifierDimension::ExplicitlySized(size_expr) => {
                        match size_expr.as_ref() {
                            Expr::IntConst(n) => Ok(*n as u32),
                            _ => Err(GlslCompileError::new(
                                "Non-literal array dimension specifiers are \
                                not supported at this time",
                            )),
                        }
                    }
                    ArraySpecifierDimension::Unsized => Err(GlslCompileError::new(
                        "Array specifier dimension for declaration \
                        cannot be unsized",
                    )),
                }?;
                Ok(a * mult)
            })
        })
        .unwrap_or(Ok(1))?;
    let decl_size = type_size.map(|ts| ts * *arr_el_count);

    Ok(Some(DeclarationInfo {
        name: name.as_ref().unwrap().to_owned().0,
        storage: storage.unwrap(),
        size: decl_size,
        location,
        explicit_location: location.is_some(),
    }))
}

fn get_block_info(block: &Block) -> Result<Option<BlockInfo>, GlslCompileError> {
    let block_name = &block.name;
    let id = &block.identifier;

    if id.is_none() {
        return Ok(None);
    }

    let instance_name = &id.as_ref().unwrap().ident.0;

    let array_dims = match &id.as_ref().unwrap().array_spec {
        Some(arr_spec) => match arr_spec.dimensions.0.len() {
            0 => None,
            1 => Some(match arr_spec.dimensions.0.first().unwrap() {
                ArraySpecifierDimension::Unsized => 0,
                ArraySpecifierDimension::ExplicitlySized(expr) => match expr.as_ref() {
                    Expr::IntConst(n) => *n as u32,
                    _ => {
                        return Err(GlslCompileError::new(
                            "Non-literal array dimension \
                        specifiers are not supported at this time",
                        ));
                    }
                },
            }),
            _ => {
                return Err(GlslCompileError::new(
                    "Multidimensional blocks are not allowed",
                ));
            }
        },
        None => None,
    };

    let mut binding: Option<u32> = None;
    let mut storage: Option<StorageQualifier> = None;
    let mut mem_layout: Option<String> = None;

    for qualifier in &block.qualifier.qualifiers {
        match qualifier {
            TypeQualifierSpec::Storage(StorageQualifier::In) => {
                return Err(GlslCompileError::new(
                    "Block storage qualifier 'in' is not supported at this time",
                ));
            }
            TypeQualifierSpec::Storage(StorageQualifier::Out) => {
                return Err(GlslCompileError::new(
                    "Block storage qualifier 'out' is not supported at this time",
                ));
            }
            TypeQualifierSpec::Storage(StorageQualifier::Buffer) => {
                return Err(GlslCompileError::new(
                    "Block storage qualifier 'buffer' is not supported at this time",
                ));
            }
            TypeQualifierSpec::Storage(qual) => {
                storage = Some(qual.to_owned());
            }
            TypeQualifierSpec::Layout(layout) => {
                for id_enum in &layout.ids {
                    match id_enum {
                        LayoutQualifierSpec::Identifier(id, expr) => {
                            if id.to_string() == LAYOUT_ID_BINDING {
                                if expr.is_none() {
                                    return Err(GlslCompileError::new(
                                        "Binding layout qualifier requires assignment",
                                    ));
                                }
                                match expr.as_ref().unwrap().as_ref() {
                                    Expr::IntConst(loc) => {
                                        if loc >= &0 {
                                            binding = Some(loc.to_owned() as u32);
                                        } else {
                                            return Err(GlslCompileError::new(
                                                "Binding layout qualifier must not be negative",
                                            ));
                                        }
                                    }
                                    Expr::UIntConst(_)
                                    | Expr::FloatConst(_)
                                    | Expr::DoubleConst(_)
                                    | Expr::BoolConst(_) => {
                                        return Err(GlslCompileError::new(
                                            "Binding layout \
                                            qualifier requires scalar integer expression",
                                        ));
                                    }
                                    _ => {
                                        return Err(GlslCompileError::new(
                                            "Non-literal \
                                            binding indices are not supported at this time",
                                        ));
                                    }
                                }
                            } else if id.to_string() == LAYOUT_ID_STD140 {
                                mem_layout = Some(LAYOUT_ID_STD140.to_string());
                            } else if id.to_string() == LAYOUT_ID_STD430 {
                                mem_layout = Some(LAYOUT_ID_STD430.to_string());
                            }
                        }
                        LayoutQualifierSpec::Shared => (),
                    };
                }
            }
            _ => (),
        }
    }

    if storage.is_none() {
        return Ok(None);
    }

    if mem_layout.is_none() || mem_layout.unwrap() != LAYOUT_ID_STD140 {
        return Err(GlslCompileError::new(
            "Memory layout 'std140' is required for blocks",
        ));
    }

    Ok(Some(BlockInfo {
        block_name: block_name.to_owned().0,
        instance_name: instance_name.to_owned(),
        array_dims,
        storage: storage.unwrap(),
        //size: type_size,
        binding,
    }))
}

fn set_decl_location(decl_type: &mut FullySpecifiedType, location: i32) {
    let new_qual = LayoutQualifierSpec::Identifier(
        Identifier(LAYOUT_ID_LOCATION.to_string()),
        Some(Box::<Expr>::new(Expr::IntConst(location))),
    );

    match &mut decl_type.qualifier {
        Some(quals) => {
            let mut target_opt: Option<&mut LayoutQualifier> = None;

            if let Some(TypeQualifierSpec::Layout(lq)) =
                (&mut quals.qualifiers).into_iter().next() {
                target_opt = Some(lq);
            }

            if let Some(target) = target_opt {
                target.ids.push(new_qual);
            } else {
                quals
                    .qualifiers
                    .push(TypeQualifierSpec::Layout(LayoutQualifier {
                        ids: NonEmpty(vec![new_qual]),
                    }));
            }
        }
        None => {
            decl_type.qualifier = Some(TypeQualifier {
                qualifiers: NonEmpty(vec![TypeQualifierSpec::Layout(LayoutQualifier {
                    ids: NonEmpty(vec![new_qual]),
                })]),
            });
        }
    };
}

// visits the version pragma and sets it to the latest GLSL version
struct PreprocessorVersionVisitor {
    fail_condition: bool,
    #[allow(dead_code)]
    fail_message: Option<String>,
}

impl PreprocessorVersionVisitor {
    fn new() -> Self {
        PreprocessorVersionVisitor {
            fail_condition: false,
            fail_message: None,
        }
    }
}

impl VisitorMut for PreprocessorVersionVisitor {
    fn visit_preprocessor_version(
        &mut self,
        decl: &mut PreprocessorVersion,
    ) -> Visit {
        if self.fail_condition {
            return Visit::Parent;
        }

        decl.version = 460;
        decl.profile = Some(PreprocessorVersionProfile::Core);

        Visit::Parent
    }
}

// visits in/out/uniform/buffer declarations with explicit layout locations
struct ScanningDeclarationVisitor<'a> {
    fail_condition: bool,
    fail_message: Option<String>,

    struct_sizes: &'a HashMap<String, Option<u32>>,

    inputs: HashMap<String, DeclarationInfo>,
    outputs: HashMap<String, DeclarationInfo>,
    uniforms: HashMap<String, DeclarationInfo>,
    buffers: HashMap<String, DeclarationInfo>,
    ubos: HashMap<String, BlockInfo>,
}

impl<'a> ScanningDeclarationVisitor<'a> {
    fn new(struct_sizes: &'a HashMap<String, Option<u32>>) -> Self {
        ScanningDeclarationVisitor {
            fail_condition: false,
            fail_message: None,
            struct_sizes,
            inputs: HashMap::new(),
            outputs: HashMap::new(),
            uniforms: HashMap::new(),
            buffers: HashMap::new(),
            ubos: HashMap::new(),
        }
    }

    fn set_failure(&mut self, message: String) {
        self.fail_condition = true;
        self.fail_message = Some(message);
    }
}

impl Visitor for ScanningDeclarationVisitor<'_> {
    fn visit_declaration(&mut self, decl: &Declaration) -> Visit {
        if self.fail_condition {
            return Visit::Parent;
        }

        if let Declaration::InitDeclaratorList(idl) = decl {
            let decl = match get_decl_info(&idl.head, self.struct_sizes) {
                Ok(opt) => match opt {
                    Some(val) => val,
                    None => {
                        return Visit::Parent;
                    }
                },
                Err(e) => {
                    self.set_failure(e.message);
                    return Visit::Parent;
                }
            };

            if decl.storage == StorageQualifier::In {
                self.inputs.insert(decl.name.clone(), decl.clone());
            } else if decl.storage == StorageQualifier::Out {
                self.outputs.insert(decl.name.clone(), decl.clone());
            } else if decl.storage == StorageQualifier::Uniform {
                self.uniforms.insert(decl.name.clone(), decl.clone());
            } else if decl.storage == StorageQualifier::Buffer {
                self.buffers.insert(decl.name.clone(), decl.clone());
            }
        } else if let Declaration::Block(block) = decl {
            let block_info = match get_block_info(block) {
                Ok(opt) => match opt {
                    Some(val) => val,
                    None => {
                        return Visit::Parent;
                    }
                },
                Err(e) => {
                    self.set_failure(e.message);
                    return Visit::Parent;
                }
            };

            if block_info.storage == StorageQualifier::Uniform {
                self.ubos.insert(block_info.block_name.clone(), block_info.clone());
            }
        }

        Visit::Parent
    }
}

// visits declarations which match output declarations from previous stage
struct MutatingDeclarationVisitor<'a> {
    fail_condition: bool,
    fail_message: Option<String>,

    struct_sizes: &'a HashMap<String, Option<u32>>,

    inputs: &'a HashMap<String, DeclarationInfo>,
    outputs: &'a HashMap<String, DeclarationInfo>,
    uniforms: &'a HashMap<String, DeclarationInfo>,
    buffers: &'a HashMap<String, DeclarationInfo>,
}

impl<'a> MutatingDeclarationVisitor<'a> {
    fn new(
        struct_sizes: &'a HashMap<String, Option<u32>>,
        inputs: &'a HashMap<String, DeclarationInfo>,
        outputs: &'a HashMap<String, DeclarationInfo>,
        uniforms: &'a HashMap<String, DeclarationInfo>,
        buffers: &'a HashMap<String, DeclarationInfo>,
    ) -> Self {
        MutatingDeclarationVisitor {
            fail_condition: false,
            fail_message: None,

            struct_sizes,

            inputs,
            outputs,
            uniforms,
            buffers,
        }
    }

    fn set_failure(&mut self, message: String) {
        self.fail_condition = true;
        self.fail_message = Some(message);
    }
}

impl VisitorMut for MutatingDeclarationVisitor<'_> {
    fn visit_declaration(&mut self, decl: &mut Declaration) -> Visit {
        if self.fail_condition {
            return Visit::Parent;
        }

        if let Declaration::InitDeclaratorList(idl) = decl {
            let decl_info = match get_decl_info(&idl.head, self.struct_sizes) {
                Ok(opt) => match opt {
                    Some(val) => val,
                    None => {
                        return Visit::Parent;
                    }
                },
                Err(e) => {
                    self.set_failure(e.message);
                    return Visit::Parent;
                }
            };

            if decl_info.location.is_some() {
                // don't need to assign location if it's explicitly set
                return Visit::Parent;
            }

            let src_map = match decl_info.storage {
                StorageQualifier::In => &self.inputs,
                StorageQualifier::Out => &self.outputs,
                StorageQualifier::Uniform => &self.uniforms,
                StorageQualifier::Buffer => &self.buffers,
                _ => {
                    return Visit::Parent;
                }
            };

            if !src_map.contains_key(&decl_info.name) {
                return Visit::Parent;
            }

            let configured_decl_info = &src_map[&decl_info.name];

            if let Some(assigned_loc) = configured_decl_info.location {
                set_decl_location(&mut idl.head.ty, assigned_loc.try_into().unwrap());
            }
        } else if let Declaration::Block(_block) = decl {
            // no-op
        }

        Visit::Parent
    }
}
