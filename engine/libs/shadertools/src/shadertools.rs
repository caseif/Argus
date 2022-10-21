#[path = "./glslang/mod.rs"] mod glslang;

use std::convert::TryInto;
use std::fmt::Write;
use std::collections::HashMap;
use std::ffi::CString;
use std::fmt;

use glsl::{parser::*, syntax::*, visitor::*};
use glslang::{*, bindings::*};

const LAYOUT_ID_LOCATION: &'static str = "location";

pub struct ProcessedGlslShader {
    stage: Stage,
    source: String,
    inputs: HashMap<String, u32>,
    outputs: HashMap<String, u32>,
    uniforms: HashMap<String, u32>,
    buffers: HashMap<String, u32>
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct GlslCompileError {
    pub message: String
}

impl GlslCompileError {
    fn new(message: &str) -> Self {
        GlslCompileError { message: message.to_string() }
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
    pub message: String
}

impl InvalidArgumentError {
    fn new(argument: &str, message: &str) -> Self {
        InvalidArgumentError { argument: argument.to_string(), message: message.to_string() }
    }
}

impl std::error::Error for InvalidArgumentError {}

impl fmt::Display for InvalidArgumentError {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "Invalid value passed for argument {} ({})", self.argument, self.message)
    }
}

struct StringWriter {
    string: String
}

impl StringWriter {
    fn new() -> Self {
        return StringWriter { string: "".to_string() };
    }

    fn to_string(&self) -> String {
        return self.string.clone();
    }
}

impl Write for StringWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.string.push_str(s);
        return Ok(());
    }
}

#[derive(Debug)]
pub struct CompiledShaderSet {
    pub bytecode: HashMap<Stage, Vec<u8>>,
    pub inputs: HashMap<String, u32>,
    pub outputs: HashMap<String, u32>,
    pub uniforms: HashMap<String, u32>,
    pub buffers: HashMap<String, u32>
}

pub fn compile_glsl_to_spirv(glsl_sources: &HashMap<Stage, String>, client: Client, client_version: TargetClientVersion,
        spirv_version: TargetLanguageVersion) -> Result<CompiledShaderSet, GlslCompileError> {
    let mut res = HashMap::<Stage, Vec<u8>>::with_capacity(glsl_sources.len());

    let processed_glsl = match process_glsl(glsl_sources) {
        Ok(v) => v,
        Err(e) => { return Err(e); }
    };

    for glsl in &processed_glsl {
        let mut program = Program::create();

        let src_c_str = CString::new(glsl.source.as_bytes()).unwrap();

        let shader_messages = Messages::none();

        glslang::initialize_process();

        let input = &Input {
            language: Source::Glsl,
            stage: glsl.stage,
            client: client,
            client_version: client_version,
            target_language: TargetLanguage::Spv,
            target_language_version: spirv_version,
            /** Shader source code */
            code: src_c_str.as_ptr(),
            default_version: 0,
            default_profile: ProfileBit::Core.into(),
            force_default_version_and_profile: 0,
            forward_compatible: 0,
            messages: shader_messages,
            resource: &DEFAULT_BUILT_IN_RESOURCE,
        };

        let mut shader = Shader::create(input);

        if !shader.preprocess(input) {
            println!("{}", shader.get_info_log());
            println!("{}", shader.get_info_debug_log());
            return Result::Err(GlslCompileError {
                message: "Failed to preprocess shader for stage ".to_owned() + &(glsl.stage as i32).to_string()
            });
        }

        if !shader.parse(input) {
            println!("{}", shader.get_info_log());
            println!("{}", shader.get_info_debug_log());
            return Result::Err(GlslCompileError {
                message: "Failed to parse shader for stage ".to_owned() + &(glsl.stage as i32).to_string()
            });
        }

        program.add_shader(shader);

        let program_messages = Messages::none();
        program.map_io();
        if !program.link(program_messages) {
            println!("{}", program.get_info_log());
            println!("{}", program.get_info_debug_log());
            return Result::Err(GlslCompileError {
                message: "Failed to link shader program".to_owned()
            });
        }

        let spirv_options = SpvOptions {
            generate_debug_info: false, // this absolutely must not be true or it'll cause glSpecializeShader to fail
            strip_debug_info: false,
            disable_optimizer: true,
            optimize_size: false,
            disassemble: false,
            validate: false,
        };

        program.spirv_generate_with_options(glsl.stage, &spirv_options);
        res.insert(glsl.stage, program.spirv_get());
    }

    let program_attrs = processed_glsl.iter()
        .find(|glsl| glsl.stage == Stage::Vertex)
        .map(|glsl| glsl.inputs.clone())
        .unwrap_or(HashMap::new());

    let program_outputs = processed_glsl.iter()
        .find(|glsl| glsl.stage == Stage::Fragment)
        .map(|glsl| glsl.outputs.clone())
        .unwrap_or(HashMap::new());

    let program_uniforms = processed_glsl.iter()
        .map(|glsl| &glsl.uniforms)
        .fold(HashMap::new(), |mut a, e| { a.extend(e.clone()); a });

    let program_buffers = processed_glsl.iter()
        .map(|glsl| &glsl.uniforms)
        .fold(HashMap::new(), |mut a, e| { a.extend(e.clone()); a });

    return Result::Ok(CompiledShaderSet {
        bytecode: res,
        inputs: program_attrs,
        outputs: program_outputs,
        uniforms: program_uniforms,
        buffers: program_buffers
    });
}

fn get_free_loc_ranges(existing_decls: Vec<DeclarationInfo>)
        -> (Vec<(u32, u32)>, u32) {
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
    for i in 0..occupied_locs.len() {
        let occupied = occupied_locs[i];
        if occupied {
            if range_start != i {
                free_ranges.push((range_start.try_into().unwrap(), (i - range_start).try_into().unwrap()));
            }
            range_start = i + 1;
        }
    }

    return (free_ranges, occupied_locs.len().try_into().unwrap());
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
    return loc;
}

fn process_glsl(glsl_sources: &HashMap<Stage, String>) -> Result<Vec<ProcessedGlslShader>, GlslCompileError> {
    let mut processed_sources = Vec::<ProcessedGlslShader>::with_capacity(glsl_sources.len());

    let mut sorted_sources: Vec<(Stage, String)> = glsl_sources.iter()
        .map(|kv| (kv.0.to_owned(), kv.1.to_owned()))
        .collect();
    sorted_sources.sort_by_key(|entry| entry.0 as u32);

    let mut struct_sizes = HashMap::<Stage, HashMap<String, Option<u32>>>::new();

    let mut pending_asts = HashMap::<Stage, TranslationUnit>::new();

    let mut inputs = HashMap::<Stage, HashMap<String, DeclarationInfo>>::new();
    let mut outputs = HashMap::<Stage, HashMap<String, DeclarationInfo>>::new();
    let mut uniforms = HashMap::<Stage, HashMap<String, DeclarationInfo>>::new();
    let mut buffers = HashMap::<Stage, HashMap<String, DeclarationInfo>>::new();

    // first pass: enumerate declarations of all shaders
    let mut i = 0;
    for kv in &sorted_sources {
        let stage = kv.0.clone();
        let source = &kv.1;

        let ast_res = ShaderStage::parse(source);
        if ast_res.is_err() {
            return Err(GlslCompileError {
                message: "Failed to parse GLSL: ".to_owned() + &ast_res.unwrap_err().info
            });
        }

        let ast = ast_res.unwrap();

        let mut struct_visitor = StructDefVisitor::new();
        ast.visit(&mut struct_visitor);

        if struct_visitor.fail_condition {
            return Err(GlslCompileError { message: struct_visitor.fail_message.unwrap() });
        }

        for ss in &struct_visitor.struct_sizes {
            println!("Struct {}: {:?}", ss.0, ss.1);
        }

        let mut scan_visitor = ScanningDeclarationVisitor::new(&struct_visitor.struct_sizes);
        ast.visit(&mut scan_visitor);

        if scan_visitor.fail_condition {
            return Err(GlslCompileError { message: scan_visitor.fail_message.unwrap() });
        }

        inputs.insert(stage, scan_visitor.inputs);
        outputs.insert(stage, scan_visitor.outputs);
        uniforms.insert(stage, scan_visitor.uniforms);
        buffers.insert(stage, scan_visitor.buffers);

        pending_asts.insert(stage, ast);

        struct_sizes.insert(stage, struct_visitor.struct_sizes);

        i += 1;
    }

    let mut all_assigned_inputs = HashMap::<Stage, HashMap::<String, DeclarationInfo>>::new();
    let mut all_assigned_outputs = HashMap::<Stage, HashMap::<String, DeclarationInfo>>::new();
    let mut all_assigned_uniforms = HashMap::<Stage, HashMap::<String, DeclarationInfo>>::new();
    let mut all_assigned_buffers = HashMap::<Stage, HashMap::<String, DeclarationInfo>>::new();

    i = 0;
    for stage in sorted_sources.iter().map(|kv| kv.0) {
        let prev_stage_opt = if i > 0 { Some(sorted_sources[i - 1].0) } else { None };
        let next_stage_opt = if i < sorted_sources.len() - 1 { Some(sorted_sources[i + 1].0) } else { None };

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
                in_decl.location = Some(get_next_free_location(in_decl.size.unwrap(), &mut avail_input_locs));
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
                out_decl.1.location = Some(get_next_free_location(out_decl.1.size.unwrap(), &mut avail_output_locs));
            }
        }

        all_assigned_inputs.insert(stage, inputs[&stage].clone());
        all_assigned_outputs.insert(stage, outputs[&stage].clone());
        all_assigned_uniforms.insert(stage, uniforms[&stage].clone());
        all_assigned_buffers.insert(stage, buffers[&stage].clone());

        i += 1;
    }

    // second pass: apply assigned locations to the AST
    for ast_kv in &mut pending_asts {
        let stage = ast_kv.0;
        let ast = ast_kv.1;

        // second pass: set locations for all input declarations which match an
        // output of the previous stage by name, and record all declarations
        // which still need to be assigned locations

        let mut mutate_visitor = MutatingDeclarationVisitor::new(&struct_sizes[&stage],
            &all_assigned_inputs[stage], &all_assigned_outputs[stage],
            &all_assigned_uniforms[stage], &all_assigned_buffers[stage]);
        ast.visit_mut(&mut mutate_visitor);

        if mutate_visitor.fail_condition {
            return Err(GlslCompileError { message: mutate_visitor.fail_message.unwrap() });
        }

        // finally, transpile the transformed shader back to GLSL

        let mut src_writer = StringWriter::new();
        ::glsl::transpiler::glsl::show_translation_unit(&mut src_writer, ast);

        let processed_src = src_writer.to_string();

        processed_sources.push(ProcessedGlslShader {
            stage: stage.to_owned(),
            source: processed_src,
            inputs: all_assigned_inputs[stage].iter()
                .filter(|(k, v)| v.location.is_some())
                .map(|(k, v)| (k.clone(), v.location.unwrap()))
                .collect(),
            outputs: all_assigned_outputs[stage].iter()
                .filter(|(k, v)| v.location.is_some())
                .map(|(k, v)| (k.clone(), v.location.unwrap()))
                .collect(),
            uniforms: all_assigned_uniforms[stage].iter()
                .filter(|(k, v)| v.location.is_some())
                .map(|(k, v)| (k.clone(), v.location.unwrap()))
                .collect(),
            buffers: all_assigned_buffers[stage].iter()
                .filter(|(k, v)| v.location.is_some())
                .map(|(k, v)| (k.clone(), v.location.unwrap()))
                .collect()
        });
    }

    return Ok(processed_sources);
}

fn get_array_size_multiplier(arr_spec_opt: &Option<ArraySpecifier>) -> Result<u32, GlslCompileError> {
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
                                    return Err(GlslCompileError::new("Array dimension specifier must be positive \
                                        non-zero integer"));
                                }
                            },
                            _ => {
                                return Err(GlslCompileError::new("Non-literal array dimension specifiers are not \
                                    supported at this time"));
                            }
                        }
                    }
                    ArraySpecifierDimension::Unsized => {
                        return Err(GlslCompileError::new("Array specifier for type must be explicitly sized"));
                    }
                };
            }

            Ok(total_size)
        },
        None => Ok(1u32)
    }
}

fn get_type_size(type_spec: &TypeSpecifierNonArray) -> Result<u32, InvalidArgumentError> {
    match type_spec {
        TypeSpecifierNonArray::Void | TypeSpecifierNonArray::Bool | TypeSpecifierNonArray::Int
            | TypeSpecifierNonArray::UInt | TypeSpecifierNonArray::Float | TypeSpecifierNonArray::Double
            | TypeSpecifierNonArray::Vec2 | TypeSpecifierNonArray::Vec3 | TypeSpecifierNonArray::Vec4
            | TypeSpecifierNonArray::DVec2
            | TypeSpecifierNonArray::BVec2 | TypeSpecifierNonArray::BVec3 | TypeSpecifierNonArray::BVec4
            | TypeSpecifierNonArray::IVec2 | TypeSpecifierNonArray::IVec3 | TypeSpecifierNonArray::IVec4
            | TypeSpecifierNonArray::UVec2 | TypeSpecifierNonArray::UVec3 | TypeSpecifierNonArray::UVec4
            => Ok(1),
        TypeSpecifierNonArray::Mat2 | TypeSpecifierNonArray::Mat23 | TypeSpecifierNonArray::Mat24 => Ok(2),
        TypeSpecifierNonArray::Mat3 | TypeSpecifierNonArray::Mat32 | TypeSpecifierNonArray::Mat34 => Ok(3),
        TypeSpecifierNonArray::Mat4 | TypeSpecifierNonArray::Mat42 | TypeSpecifierNonArray::Mat43 => Ok(4),
        TypeSpecifierNonArray::DMat2 | TypeSpecifierNonArray::DMat23 | TypeSpecifierNonArray::DMat24 => Ok(4),
        TypeSpecifierNonArray::DMat3 | TypeSpecifierNonArray::DMat32 | TypeSpecifierNonArray::DMat34 => Ok(6),
        TypeSpecifierNonArray::DMat4 | TypeSpecifierNonArray::DMat42 | TypeSpecifierNonArray::DMat43 => Ok(8),
        TypeSpecifierNonArray::DVec3 | TypeSpecifierNonArray::DVec4 => Ok(2),
        TypeSpecifierNonArray::TypeName(_) =>
            Err(InvalidArgumentError::new("type_spec", "Cannot get size of non-builtin type")),
        _ => Ok(0)
    }
}

struct StructDefVisitor {
    fail_condition: bool,
    fail_message: Option<String>,
    struct_sizes: HashMap<String, Option<u32>>
}

impl StructDefVisitor {
    fn new() -> Self {
        StructDefVisitor {
            fail_condition: false,
            fail_message: None,
            struct_sizes: HashMap::new()
        }
    }
}

impl Visitor for StructDefVisitor {
    fn visit_struct_specifier(&mut self, struct_spec: &::glsl::syntax::StructSpecifier) ->Visit {
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
                let size = self.struct_sizes.get(&tn.0);
                if size.is_some() {
                    size.unwrap().unwrap_or(0)
                } else {
                    self.fail_condition = true;
                    self.fail_message = Some("Unknown type name ".to_string() + &tn.0 + " referenced in struct");
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
                self.struct_sizes.insert(struct_spec.name.as_ref().unwrap().0.to_owned(), None);
                return Visit::Parent;
            }

            total_size += base_size * arr_mult;
        }

        self.struct_sizes.insert(struct_spec.name.as_ref().unwrap().0.to_owned(), Some(total_size));

        return Visit::Parent;
    }
}

#[derive(Clone)]
struct DeclarationInfo {
    name: String,
    storage: StorageQualifier,
    size: Option<u32>,
    location: Option<u32>,
    explicit_location: bool
}

fn get_decl_info(single_decl: &SingleDeclaration, struct_sizes: &HashMap<String, Option<u32>>)
        -> Result<Option<DeclarationInfo>, GlslCompileError> {
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

    let mut storage: Option::<StorageQualifier> = None;

    for qualifier in &qualifiers.as_ref().unwrap().qualifiers {
        match qualifier {
            TypeQualifierSpec::Storage(StorageQualifier::Attribute) => {
                return Err(GlslCompileError::new("Storage qualifier 'attribute' is not supported"));
            },
            TypeQualifierSpec::Storage(StorageQualifier::Varying) => {
                return Err(GlslCompileError::new("Storage qualifier 'varying' is not supported"));
            },
            TypeQualifierSpec::Storage(qual) => {
                storage = Some(qual.to_owned());
            },
            TypeQualifierSpec::Layout(layout) => {
                for id_enum in &layout.ids {
                    match id_enum {
                        LayoutQualifierSpec::Identifier(id, expr) => {
                            if id.to_string() == LAYOUT_ID_LOCATION {
                                if expr.is_none() {
                                    return Err(GlslCompileError::new("Location layout qualifier requires assignment"));
                                }
                                match expr.as_ref().unwrap().as_ref() {
                                    Expr::IntConst(loc) => {
                                        if loc >= &0 {
                                            location = Some(loc.to_owned() as u32);
                                        } else {
                                            return Err(GlslCompileError::new("Location layout qualifier must not be negative"))
                                        }
                                    },
                                    Expr::UIntConst(_) | Expr::FloatConst(_) | Expr::DoubleConst(_) | Expr::BoolConst(_) => {
                                        return Err(GlslCompileError::new("Location layout qualifier requires scalar integer expression"));
                                    },
                                    _ => {
                                        return Err(GlslCompileError::new("Non-literal layout IDs are not supported at this time"));
                                    }
                                }
                            }
                        },
                        LayoutQualifierSpec::Shared => ()
                    };
                }
            }
            _ => ()
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
                v => Some(v)
            }
        } else {
            return Err(GlslCompileError::new(std::format!("Unknown type name {} referenced in declaration '{}'",
                &tn.0, name.as_ref().unwrap().0).as_str()));
        }
    } else {
        Some(get_type_size(&decl_type.ty.ty).unwrap())
    };

    return Ok(Some(DeclarationInfo {
        name: name.as_ref().unwrap().to_owned().0,
        storage: storage.unwrap(),
        size: type_size,
        location: location,
        explicit_location: location.is_some()
    }));
}

fn set_decl_location(decl_type: &mut FullySpecifiedType, location: i32) {
    let new_qual = LayoutQualifierSpec::Identifier(Identifier(LAYOUT_ID_LOCATION.to_string()),
        Some(Box::<Expr>::new(Expr::IntConst(location))));

    match &mut decl_type.qualifier {
        Some(quals) => {
            let mut target_opt: Option::<&mut LayoutQualifier> = None;

            for qual in &mut quals.qualifiers {
                if let TypeQualifierSpec::Layout(lq) = qual {
                    target_opt = Some(lq);
                }
                break;
            }

            if let Some(target) = target_opt {
                target.ids.push(new_qual);
            } else {
                quals.qualifiers.push(TypeQualifierSpec::Layout(LayoutQualifier {
                    ids: NonEmpty(vec![new_qual])
                }));
            }
        },
        None => {
            decl_type.qualifier = Some(TypeQualifier {
                qualifiers: NonEmpty(vec![TypeQualifierSpec::Layout(
                    LayoutQualifier {
                        ids: NonEmpty(vec![new_qual])
                    }
                )])
            });
        }
    };
}

// visits in/out/uniform/buffer declarations with explicit layout locations
struct ScanningDeclarationVisitor<'a> {
    fail_condition: bool,
    fail_message: Option<String>,

    struct_sizes: &'a HashMap<String, Option<u32>>,

    inputs: HashMap<String, DeclarationInfo>,
    outputs: HashMap<String, DeclarationInfo>,
    uniforms: HashMap<String, DeclarationInfo>,
    buffers: HashMap<String, DeclarationInfo>
}

impl<'a> ScanningDeclarationVisitor<'a> {
    fn new(struct_sizes: &'a HashMap<String, Option<u32>>) -> Self {
        ScanningDeclarationVisitor {
            fail_condition: false,
            fail_message: None,

            struct_sizes: struct_sizes,

            inputs: HashMap::new(),
            outputs: HashMap::new(),
            uniforms: HashMap::new(),
            buffers: HashMap::new(),
        }
    }

    fn set_failure(&mut self, message: String) {
        self.fail_condition = true;
        self.fail_message = Some(message);
    }
}

impl<'a> Visitor for ScanningDeclarationVisitor<'a> {
    fn visit_declaration(&mut self, decl: &::glsl::syntax::Declaration) -> Visit {
        if self.fail_condition {
            return Visit::Parent;
        }

        if let Declaration::InitDeclaratorList(idl) = decl {
            let decl = match get_decl_info(&idl.head, &self.struct_sizes) {
                Ok(opt) => match opt {
                    Some(val) => val,
                    None => {
                        return Visit::Parent;
                    }
                },
                Err(e) => {
                    self.set_failure(e.message);
                    return Visit::Parent;
                },
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
            println!("Visiting block {:?}", block.name);
        }

        return Visit::Parent;
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
    buffers: &'a HashMap<String, DeclarationInfo>
}

impl<'a> MutatingDeclarationVisitor<'a> {
    fn new(struct_sizes: &'a HashMap<String, Option<u32>>, inputs: &'a HashMap<String, DeclarationInfo>,
            outputs: &'a HashMap<String, DeclarationInfo>, uniforms: &'a HashMap<String, DeclarationInfo>,
            buffers: &'a HashMap<String, DeclarationInfo>) -> Self {
        MutatingDeclarationVisitor {
            fail_condition: false,
            fail_message: None,

            struct_sizes,

            inputs,
            outputs,
            uniforms,
            buffers
        }
    }

    fn set_failure(&mut self, message: String) {
        self.fail_condition = true;
        self.fail_message = Some(message);
    }
}

impl<'a> VisitorMut for MutatingDeclarationVisitor<'a> {
    fn visit_declaration(&mut self, decl: &mut ::glsl::syntax::Declaration) -> Visit {
        if self.fail_condition {
            return Visit::Parent;
        }

        if let Declaration::InitDeclaratorList(idl) = decl {
            let decl_info = match get_decl_info(&idl.head, &self.struct_sizes) {
                Ok(opt) => match opt {
                    Some(val) => val,
                    None => {
                        return Visit::Parent;
                    }
                },
                Err(e) => {
                    self.set_failure(e.message);
                    return Visit::Parent;
                },
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
                _ => { return Visit::Parent; }
            };

            if !src_map.contains_key(&decl_info.name) {
                return Visit::Parent;
            }

            let configured_decl_info = &src_map[&decl_info.name];

            if let Some(assigned_loc) = configured_decl_info.location {
                set_decl_location(&mut idl.head.ty, assigned_loc.try_into().unwrap());
            }
        } else if let Declaration::Block(block) = decl {
            println!("Visiting block {:?}", block.name);
        }

        return Visit::Parent;
    }
}
