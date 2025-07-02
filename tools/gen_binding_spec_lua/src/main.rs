extern crate argus_modules;

use argus_logging::{LogLevel, LogManager, LogSettings, PreludeComponent};
use argus_scripting_bind::*;
use clap::Parser;
use std::any::TypeId;
use std::collections::HashMap;
use std::fs::OpenOptions;
use std::io;
use std::io::Write;

fn main() {
    LogManager::initialize(LogSettings {
        prelude: vec![PreludeComponent::Level],
        min_level: LogLevel::Severe,
        ..LogSettings::default()
    }).expect("Unable to initialize logger");

    let args = Cli::parse();
    let mut out: Box<dyn Write> = if let Some(out_file) = args.out_file {
        Box::new(
            OpenOptions::new()
                .create(true)
                .truncate(true)
                .read(false)
                .write(true)
                .open(out_file)
                .expect("Unable to write to file")
        )
    } else {
        Box::new(io::stdout())
    };

    emit_defs(&mut out).unwrap();
}

#[derive(Parser)]
#[command(version, about, long_about = None)]
struct Cli {
    #[arg(value_name = "out file")]
    out_file: Option<String>,
}

fn emit_defs(out: &mut impl Write) -> io::Result<()> {
    //register_script_bindings();

    out.write_all(b"---@meta\n\n")?;
    out.write_all(
        format!("---@class {0}\nlocal {0} = {{}}\n\n", ENGINE_LUA_NAMESPACE)
            .as_bytes()
    )?;

    let mut fields = HashMap::<TypeId, Vec<&BoundFieldInfo>>::new();
    let mut global_fns = Vec::<&BoundFunctionInfo>::new();
    let mut assoc_fns = HashMap::<TypeId, Vec<&BoundFunctionInfo>>::new();

    for (field_def, _) in BOUND_FIELD_DEFS {
        fields.entry((field_def.containing_type)()).or_default().push(field_def);
    }
    for (fn_def, _) in BOUND_FUNCTION_DEFS {
        if fn_def.ty == FunctionType::Global {
            global_fns.push(fn_def);
        } else {
            assoc_fns.entry(fn_def.assoc_type.unwrap()()).or_default().push(fn_def);
        }
    }
    for struct_def in BOUND_STRUCT_DEFS {
        let struct_type_id = (struct_def.type_id)();
        let fields = fields.entry(struct_type_id).or_default();
        emit_struct_def(out, struct_def, fields)?;

        if let Some(struct_fns) = assoc_fns.get(&struct_type_id) {
            for fn_def in struct_fns {
                emit_function_def(out, fn_def, Some(struct_def))?;
            }
        }
    }

    for enum_def in BOUND_ENUM_DEFS {
        emit_enum_def(out, enum_def)?;
    }

    for fn_def in global_fns {
        emit_function_def(out, fn_def, None)?;
    }

    Ok(())
}

fn emit_struct_def(out: &mut impl Write, def: &BoundStructInfo, fields: &Vec<&BoundFieldInfo>)
    -> io::Result<()> {
    out.write_all(format!("---@class {}\n", def.name).as_bytes())?;
    for field in fields {
        let ty = serde_json::from_str::<ObjectType>(field.type_serial)?;
        out.write_all(format!("---@field {} {}\n", field.name, get_lua_type(&ty)).as_bytes())?;
    }
    out.write_all(format!("local {} = {{}}\n\n", def.name).as_bytes())?;
    Ok(())
}

fn emit_enum_def(out: &mut impl Write, def: &BoundEnumInfo) -> io::Result<()> {
    out.write_all(format!("---@alias {}\n", def.name).as_bytes())?;
    for val in def.values {
        out.write_all(format!("---| {} # {}\n", (val.1)(), val.0).as_bytes())?;
    }
    out.write_all(b"\n")?;

    out.write_all(format!("---@class {}Enum\n", def.name).as_bytes())?;
    for val in def.values {
        out.write_all(format!("---@field {} {}\n", val.0, def.name).as_bytes())?;
    }
    out.write_all(b"\n")?;

    out.write_all(format!("{} = {{\n", def.name).as_bytes())?;
    for val in def.values {
        out.write_all(format!("    {} = {},\n", val.0, (val.1)()).as_bytes())?;
    }
    out.write_all(b"}\n\n")?;
    Ok(())
}

fn emit_function_comment(out: &mut impl Write, def: &BoundFunctionInfo) -> io::Result<()> {
    for i in 0..def.param_names.len() {
        let ty_index =
            if def.ty == FunctionType::MemberInstance || def.ty == FunctionType::Extension {
                i + 1
            } else {
                i
            };
        let name = def.param_names[i];
        let ty: ObjectType = serde_json::de::from_str(def.param_type_serials[ty_index])?;
        out.write_all(format!("---@param {} {}\n", name, get_lua_type(&ty)).as_bytes())?;
    }

    let return_ty: ObjectType = serde_json::de::from_str(def.return_type_serial)?;
    if return_ty.ty != IntegralType::Empty {
        out.write_all(format!("---@return {}\n", get_lua_type(&return_ty)).as_bytes())?;
    }
    Ok(())
}

fn emit_function_def(
    out: &mut impl Write,
    fn_def: &BoundFunctionInfo,
    struct_def: Option<&BoundStructInfo>
) -> io::Result<()> {
    emit_function_comment(out, fn_def)?;
    out.write_all(
        format!("function {}(", get_function_name(fn_def, struct_def)).as_bytes()
    )?;
    for i in 0..fn_def.param_names.len() {
        out.write_all(fn_def.param_names[i].as_bytes())?;
        if i != fn_def.param_names.len() - 1 {
            out.write_all(b", ")?;
        }
    }
    out.write_all(b") end\n\n")?;

    Ok(())
}

fn get_function_name(fn_def: &BoundFunctionInfo, struct_def: Option<&BoundStructInfo>) -> String{
    match fn_def.ty {
        FunctionType::Global =>
            format!("{}.{}", ENGINE_LUA_NAMESPACE, fn_def.name),
        FunctionType::MemberStatic =>
            format!("{}.{}", struct_def.unwrap().name, fn_def.name),
        FunctionType::MemberInstance |
        FunctionType::Extension =>
            format!("{}:{}", struct_def.unwrap().name, fn_def.name),
    }
}

fn get_lua_type(ty: &ObjectType) -> &str {
    match ty.ty {
        IntegralType::Empty => "nil",
        IntegralType::Int8 |
        IntegralType::Int16 |
        IntegralType::Int32 |
        IntegralType::Int64 |
        IntegralType::Int128 |
        IntegralType::Uint8 |
        IntegralType::Uint16 |
        IntegralType::Uint32 |
        IntegralType::Uint64 |
        IntegralType::Uint128 |
        IntegralType::Float32 |
        IntegralType::Float64 => "number",
        IntegralType::Boolean => "boolean",
        IntegralType::String => "string",
        IntegralType::Object => ty.type_name.as_ref().unwrap(),
        IntegralType::Enum => ty.type_name.as_ref().unwrap(),
        IntegralType::Reference => ty.primary_type.as_ref().unwrap().type_name.as_ref().unwrap(),
        IntegralType::Vec |
        IntegralType::VecRef => "table",
        IntegralType::Result => "Result",
        IntegralType::Callback => "function",
    }
}
