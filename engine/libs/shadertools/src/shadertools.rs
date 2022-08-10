#[path = "./glslang/mod.rs"] mod glslang;

use std::collections::HashMap;
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

impl std::error::Error for GlslCompileError {}

impl fmt::Display for GlslCompileError {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "error: {}", self.message)
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

    let mut inputs = HashMap::<Stage, Vec<(String, Option::<u32>)>>::new();
    let mut outputs = HashMap::<Stage, Vec<(String, Option::<u32>)>>::new();
    let mut uniforms = HashMap::<String, Option::<u32>>::new();

    // first pass: enumerate inputs/outputs/uniforms of all shaders
    for kv in glsl_sources {
        let stage = kv.0;
        let source = kv.1;

        let ast = ShaderStage::parse(source);
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

        let mut decl_visitor = DeclarationEnumerateVisitor::new(struct_visitor.struct_sizes);
        ast.as_ref().unwrap().visit(&mut decl_visitor);

        if decl_visitor.fail_condition {
            return Err(GlslCompileError { message: decl_visitor.fail_message.unwrap() });
        }

        println!("inputs:");
        for input in decl_visitor.inputs {
            println!("  {} @ {:?}", input.0, input.1);
        }
        println!("outputs:");
        for output in decl_visitor.outputs {
            println!("  {} @ {:?}", output.0, output.1);
        }
        println!("uniforms:");
        for uniform in decl_visitor.uniforms {
            println!("  {} @ {:?}", uniform.0, uniform.1);
        }

        processed_sources.push(ProcessedGlslShader {
            stage: kv.0.to_owned(),
            source: kv.1.to_owned(),
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
                let dim_size = match dim {
                    ArraySpecifierDimension::ExplicitlySized(size_expr) => {
                        match size_expr.as_ref() {
                            Expr::IntConst(size) => {
                                if *size > 0 {
                                    Ok(*size as u32)
                                } else {
                                    Err(GlslCompileError { message: "Array dimension specifier must be positive non-zero integer".to_owned() })
                                }
                            },
                            _ => {
                                Err(GlslCompileError { message: "Non-literal array dimension specifiers are not supported at this time".to_owned() })
                            }
                        }
                    }
                    ArraySpecifierDimension::Unsized => {
                        Err(GlslCompileError { message: "Array specifier for type must be explicitly sized".to_owned() })
                    }
                };
            }

            Ok(total_size)
        },
        None => Ok(1u32)
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
            let base_size = match &ts.ty {
                TypeSpecifierNonArray::Void | TypeSpecifierNonArray::Bool | TypeSpecifierNonArray::Int
                    | TypeSpecifierNonArray::UInt | TypeSpecifierNonArray::Float | TypeSpecifierNonArray::Double
                    | TypeSpecifierNonArray::Vec2 | TypeSpecifierNonArray::Vec3 | TypeSpecifierNonArray::Vec4
                    | TypeSpecifierNonArray::DVec2
                    | TypeSpecifierNonArray::BVec2 | TypeSpecifierNonArray::BVec3 | TypeSpecifierNonArray::BVec4
                    | TypeSpecifierNonArray::IVec2 | TypeSpecifierNonArray::IVec3 | TypeSpecifierNonArray::IVec4
                    | TypeSpecifierNonArray::UVec2 | TypeSpecifierNonArray::UVec3 | TypeSpecifierNonArray::UVec4
                    => 1,
                TypeSpecifierNonArray::Mat2 | TypeSpecifierNonArray::Mat23 | TypeSpecifierNonArray::Mat24 => 2,
                TypeSpecifierNonArray::Mat3 | TypeSpecifierNonArray::Mat32 | TypeSpecifierNonArray::Mat34 => 3,
                TypeSpecifierNonArray::Mat4 | TypeSpecifierNonArray::Mat42 | TypeSpecifierNonArray::Mat43 => 4,
                TypeSpecifierNonArray::DMat2 | TypeSpecifierNonArray::DMat23 | TypeSpecifierNonArray::DMat24 => 4,
                TypeSpecifierNonArray::DMat3 | TypeSpecifierNonArray::DMat32 | TypeSpecifierNonArray::DMat34 => 6,
                TypeSpecifierNonArray::DMat4 | TypeSpecifierNonArray::DMat42 | TypeSpecifierNonArray::DMat43 => 8,
                TypeSpecifierNonArray::DVec3 | TypeSpecifierNonArray::DVec4 => 2,
                TypeSpecifierNonArray::TypeName(tn) => {
                    let size = self.struct_sizes.get(&tn.0);
                    if size.is_some() {
                        size.unwrap().unwrap_or(0)
                    } else {
                        self.fail_condition = true;
                        self.fail_message = Some("Unknown type name ".to_string() + &tn.0 + " referenced in struct");
                        0u32
                    }

                },
                // struct is opaque
                _ => 0
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

struct DeclarationEnumerateVisitor {
    fail_condition: bool,
    fail_message: Option<String>,
    struct_lens: HashMap<String, Option<u32>>,
    inputs: Vec<(String, Option<u32>)>,
    outputs: Vec<(String, Option<u32>)>,
    uniforms: Vec<(String, Option<u32>)>,
    buffers: Vec<(String, Option<u32>)>
}

impl DeclarationEnumerateVisitor {
    fn new(struct_lens: HashMap<String, Option<u32>>) -> Self {
        DeclarationEnumerateVisitor {
            fail_condition: false,
            fail_message: None,
            struct_lens: struct_lens,
            inputs: Vec::new(),
            outputs: Vec::new(),
            uniforms: Vec::new(),
            buffers: Vec::new(),
        }
    }
}

impl Visitor for DeclarationEnumerateVisitor {
    fn visit_declaration(&mut self, decl: &::glsl::syntax::Declaration) -> Visit {
        if self.fail_condition {
            return Visit::Parent;
        }

        if let Declaration::InitDeclaratorList(idl) = decl {
            let single_decl = &idl.head;
            
            println!("Visiting declaration for {:?}", single_decl.name);

            let name = &single_decl.name;

            if name.is_none() {
                return Visit::Parent;
            }

            let decl_type = &single_decl.ty;
            let qualifiers = &decl_type.qualifier;
            if qualifiers.is_none() {
                // not interested if it's not in/out/uniform/buffer
                return Visit::Parent;
            }

            let mut target_vec: Option<&mut Vec<(String, Option<u32>)>> = None;
            let mut location: Option<u32> = None;
            for qualifier in &qualifiers.as_ref().unwrap().qualifiers {
                match qualifier {
                    TypeQualifierSpec::Storage(StorageQualifier::In) => {
                        target_vec = Some(&mut self.inputs);
                    },
                    TypeQualifierSpec::Storage(StorageQualifier::Out) => {
                        target_vec = Some(&mut self.outputs);
                    },
                    TypeQualifierSpec::Storage(StorageQualifier::Uniform) => {
                        target_vec = Some(&mut self.uniforms);
                    },
                    TypeQualifierSpec::Storage(StorageQualifier::Buffer) => {
                        target_vec = Some(&mut self.buffers);
                    },
                    TypeQualifierSpec::Storage(StorageQualifier::Attribute) => {
                        self.fail_condition = true;
                        self.fail_message = Some("Storage qualifier 'attribute' is not supported".to_owned());
                    },
                    TypeQualifierSpec::Storage(StorageQualifier::Varying) => {
                        self.fail_condition = true;
                        self.fail_message = Some("Storage qualifier 'varying' is not supported".to_owned());
                    },
                    TypeQualifierSpec::Layout(layout) => {
                        for id_enum in &layout.ids {
                            match id_enum {
                                LayoutQualifierSpec::Identifier(id, expr) => {
                                    if id.to_string() == LAYOUT_ID_LOCATION {
                                        if expr.is_none() {
                                            self.fail_condition = true;
                                            self.fail_message = Some("Location layout qualifier requires assignment".to_owned());
                                        }
                                        match expr.as_ref().unwrap().as_ref() {
                                            Expr::IntConst(loc) => {
                                                if loc >= &0 {
                                                    location = Some(loc.to_owned() as u32);
                                                } else {
                                                    self.fail_condition = true;
                                                    self.fail_message = Some("Location layout qualifier must not be negative".to_owned());
                                                }
                                            },
                                            Expr::UIntConst(_) | Expr::FloatConst(_) | Expr::DoubleConst(_) | Expr::BoolConst(_) => {
                                                self.fail_condition = true;
                                                self.fail_message = Some("Location layout qualifier requires scalar integer expression".to_owned());
                                            },
                                            _ => {
                                                self.fail_condition = true;
                                                self.fail_message = Some("Non-literal layout IDs are not supported at this time".to_owned());
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

            if target_vec.is_some() {
                target_vec.unwrap().push((name.as_ref().unwrap().0.to_owned(), location));
            }
        } else if let Declaration::Block(block) = decl {
            println!("Visiting block {:?}", block.name);
        }

        return Visit::Parent;
    }
}

struct InOutDeclarationWithExplicitLocationVisitor {

}
