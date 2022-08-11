#[path = "./glslang/mod.rs"] mod glslang;

use std::fmt::Write;
use std::{collections::HashMap, io::BufWriter};
use std::ffi::CString;
use std::fmt;

use glsl::{parser::*, syntax::*, transpiler::*, visitor::*};
use glslang::{*, bindings::*};

const LAYOUT_ID_LOCATION: &'static str = "location";

pub struct ProcessedGlslShader {
    stage: Stage,
    source: String,
    attributes: HashMap<String, u32>,
    uniforms: HashMap<String, u32>
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

pub fn compile_glsl_to_spirv(glsl_sources: &HashMap<Stage, String>, client: Client, client_version: TargetClientVersion,
        spirv_version: TargetLanguageVersion) -> Result<HashMap<Stage, Vec<u8>>, GlslCompileError> {
    let mut res = HashMap::<Stage, Vec<u8>>::with_capacity(glsl_sources.len());

    let processed_glsl = process_glsl(glsl_sources);
    if processed_glsl.is_err() {
        return Err(processed_glsl.err().unwrap());
    }

    for glsl in processed_glsl.unwrap() {
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

    return Result::Ok(res);
}

fn process_glsl(glsl_sources: &HashMap<Stage, String>) -> Result<Vec<ProcessedGlslShader>, GlslCompileError> {
    let mut processed_sources = Vec::<ProcessedGlslShader>::with_capacity(glsl_sources.len());

    let mut attributes = HashMap::<String, u32>::new();
    let mut uniforms = HashMap::<String, u32>::new();
    let mut buffers = HashMap::<String, u32>::new();

    let mut sorted_sources: Vec<(Stage, String)> = glsl_sources.iter()
        .map(|kv| (kv.0.to_owned(), kv.1.to_owned()))
        .collect();
    sorted_sources.sort_by_key(|entry| entry.0 as u32);

    let mut prev_outputs: Option::<HashMap<String, DeclarationInfo>> = None;

    // first pass: enumerate inputs/outputs/uniforms of all shaders
    for kv in sorted_sources {
        let stage = &kv.0;
        let source = &kv.1;

        let mut ast = ShaderStage::parse(source);
        if ast.is_err() {
            return Err(GlslCompileError {
                message: "Failed to parse GLSL: ".to_owned() + &ast.unwrap_err().info
            });
        }

        let mut struct_visitor = StructDefVisitor::new();
        ast.as_ref().unwrap().visit(&mut struct_visitor);

        if struct_visitor.fail_condition {
            return Err(GlslCompileError { message: struct_visitor.fail_message.unwrap() });
        }

        for ss in &struct_visitor.struct_sizes {
            println!("Struct {}: {:?}", ss.0, ss.1);
        }

        let mut all_outs = HashMap::<String, DeclarationInfo>::new();

        // first pass

        let mut first_visitor = ExplicitlyLocatedDeclarationVisitor::new(&prev_outputs, &struct_visitor.struct_sizes);
        ast.as_mut().unwrap().visit_mut(&mut first_visitor);

        if first_visitor.fail_condition {
            return Err(GlslCompileError { message: first_visitor.fail_message.unwrap() });
        }

        all_outs.reserve(first_visitor.outputs.len());
        for out in first_visitor.outputs {
            all_outs.insert(out.0, out.1);
        }

        // second pass

        let mut second_visitor = MatchedInputDeclarationVisitor::new(&prev_outputs, &struct_visitor.struct_sizes);
        ast.as_mut().unwrap().visit_mut(&mut second_visitor);

        if second_visitor.fail_condition {
            return Err(GlslCompileError { message: second_visitor.fail_message.unwrap() });
        }

        all_outs.reserve(second_visitor.outputs.len());
        for out in second_visitor.outputs {
            all_outs.insert(out.0, out.1);
        }

        // third pass
        //TODO

        prev_outputs = Some(all_outs);

        let mut src_writer = StringWriter::new();
        ::glsl::transpiler::glsl::show_translation_unit(&mut src_writer, ast.as_ref().unwrap());

        let processed_src = src_writer.to_string();

        processed_sources.push(ProcessedGlslShader {
            stage: kv.0.to_owned(),
            source: processed_src,
            attributes: HashMap::new(),
            uniforms: HashMap::new()
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
        TypeSpecifierNonArray::TypeName(tn) =>
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
                println!("path 1");
            } else {
                quals.qualifiers.push(TypeQualifierSpec::Layout(LayoutQualifier {
                    ids: NonEmpty(vec![new_qual])
                }));
                println!("path 2");
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
            println!("path 3");
        }
    };
}

#[macro_export]
macro_rules! decl_visitor {
    (
        $name: ident
        $closure: expr
    ) => {
        #[allow(dead_code)]
        struct $name<'a> {
            fail_condition: bool,
            fail_message: Option<String>,
        
            struct_sizes: &'a HashMap<String, Option<u32>>,
            prev_stage_outputs: &'a Option::<HashMap<String, DeclarationInfo>>,
        
            inputs: HashMap<String, DeclarationInfo>,
            outputs: HashMap<String, DeclarationInfo>,
            uniforms: HashMap<String, DeclarationInfo>,
            buffers: HashMap<String, DeclarationInfo>
        }

        impl<'a> $name<'a> {
            #[allow(dead_code)]
            fn new(prev_stage_outputs: &'a Option::<HashMap<String, DeclarationInfo>>,
                    struct_sizes: &'a HashMap<String, Option<u32>>) -> Self {
                $name {
                    fail_condition: false,
                    fail_message: None,
        
                    struct_sizes: struct_sizes,
                    prev_stage_outputs: prev_stage_outputs,
        
                    inputs: HashMap::new(),
                    outputs: HashMap::new(),
                    uniforms: HashMap::new(),
                    buffers: HashMap::new(),
                }
            }

            #[allow(dead_code)]
            fn set_failure(&mut self, message: String) {
                self.fail_condition = true;
                self.fail_message = Some(message);
            }
        }

        impl<'a> VisitorMut for $name<'a> {
            fn visit_declaration(&mut self, decl: &mut ::glsl::syntax::Declaration) -> Visit {
                let x: &dyn Fn(&mut Self, &mut ::glsl::syntax::Declaration) -> Visit = &$closure;
                x(self, decl)
            }
        }
    }
}

// visits in/out/uniform/buffer declarations with explicit layout locations
decl_visitor!(ExplicitlyLocatedDeclarationVisitor |this, decl| {
    if this.fail_condition {
        return Visit::Parent;
    }

    if let Declaration::InitDeclaratorList(idl) = decl {
        let decl = match get_decl_info(&idl.head, &this.struct_sizes) {
            Ok(opt) => match opt {
                Some(val) => val,
                None => {
                    return Visit::Parent;
                }
            },
            Err(e) => {
                this.set_failure(e.message);
                return Visit::Parent;
            },
        };

        if decl.storage == StorageQualifier::In {
            if decl.location.is_none() {
                // only interested in explicitly-located inputs for this pass
                return Visit::Parent;
            }

            if let Some(outs) = &this.prev_stage_outputs {
                if outs.contains_key(&decl.name) && !outs[&decl.name].explicit_location {
                    this.set_failure(format!("Input '{}' with explicit location matches implicitly-located \
                        output of previous stage", decl.name).to_owned());
                }
            }

            this.inputs.insert(decl.name.clone(), decl.clone());
        } else if decl.storage == StorageQualifier::Out {
            if decl.location.is_none() {
                // only interested in explicitly-located outputs for this pass
                return Visit::Parent;
            }

            this.outputs.insert(decl.name.clone(), decl.clone());
        }
        //TODO: handle other storage types
    } else if let Declaration::Block(block) = decl {
        println!("Visiting block {:?}", block.name);
    }

    return Visit::Parent;
});

// visits in declarations which match output declarations from previous stage
decl_visitor!(MatchedInputDeclarationVisitor |this, decl| {
    if this.fail_condition {
        return Visit::Parent;
    }

    if let Declaration::InitDeclaratorList(idl) = decl {
        let mut decl_info = match get_decl_info(&idl.head, &this.struct_sizes) {
            Ok(opt) => match opt {
                Some(val) => val,
                None => {
                    return Visit::Parent;
                }
            },
            Err(e) => {
                this.set_failure(e.message);
                return Visit::Parent;
            },
        };

        if decl_info.storage == StorageQualifier::In {
            if decl_info.location.is_some() {
                // only interested in implicitly-located inputs for this pass
                return Visit::Parent;
            }

            println!("second pass: input");

            if let Some(outs) = &this.prev_stage_outputs {
                println!("we have outputs");
                println!("checking {}", decl_info.name);
                for out in outs {
                    println!("candidate: {}", out.0);
                }

                if outs.contains_key(&decl_info.name) {
                    let matching_out = &outs[&decl_info.name];
                    println!("input matches output, location {:?}", matching_out.location);
                    if matching_out.explicit_location {
                        this.set_failure("Input with implicit location matches explicitly-located \
                            output of previous stage".to_owned());
                    }

                    decl_info.location = matching_out.location;

                    let decl_type = &mut idl.head.ty;
                    
                    set_decl_location(decl_type, matching_out.location.unwrap() as i32);
                }
            }

            this.inputs.insert(decl_info.name.clone(), decl_info.clone());
        }
        //TODO: handle other storage types
    } else if let Declaration::Block(block) = decl {
        println!("Visiting block {:?}", block.name);
    }

    return Visit::Parent;
});

// visits all remaining in/out/uniform/buffer declarations
decl_visitor!(ImplicitlyLocatedDeclarationVisitor |this, decl| {
    if this.fail_condition {
        return Visit::Parent;
    }

    if let Declaration::InitDeclaratorList(idl) = decl {
        let mut decl = match get_decl_info(&idl.head, &this.struct_sizes) {
            Ok(opt) => match opt {
                Some(val) => val,
                None => {
                    return Visit::Parent;
                }
            },
            Err(e) => {
                this.set_failure(e.message);
                return Visit::Parent;
            },
        };

        if decl.storage == StorageQualifier::In {
            if decl.location.is_some() {
                // only interested in implicitly-located inputs for this pass
                return Visit::Parent;
            }

            if let Some(outs) = &this.prev_stage_outputs {
                if outs.contains_key(&decl.name) {
                    let matching_out = &outs[&decl.name];
                    if matching_out.explicit_location {
                        this.set_failure("Input with implicit location matches explicitly-located \
                            output of previous stage".to_owned());
                    }

                    decl.location = matching_out.location;
                }
            }

            this.inputs.insert(decl.name.clone(), decl.clone());
        }
        //TODO: handle other storage types
    } else if let Declaration::Block(block) = decl {
        println!("Visiting block {:?}", block.name);
    }

    return Visit::Parent;
});
