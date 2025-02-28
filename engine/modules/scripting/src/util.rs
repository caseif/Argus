use argus_scripting_bind::{FunctionType, IntegralType};

pub fn get_qualified_field_name(type_name: impl AsRef<str>, field_name: impl AsRef<str>) -> String {
    format!("{}::{}", type_name.as_ref(), field_name.as_ref())
}

pub fn get_qualified_function_name(
    fn_type: FunctionType,
    fn_name: impl AsRef<str>,
    type_name: Option<impl AsRef<str>>,
) -> String {
    match fn_type {
        FunctionType::Global => fn_name.as_ref().to_string(),
        FunctionType::MemberInstance |
        FunctionType::Extension =>
            format!("{}#{}", type_name.unwrap().as_ref(), fn_name.as_ref()),
        FunctionType::MemberStatic =>
            format!("{}::{}", type_name.unwrap().as_ref(), fn_name.as_ref()),
    }
}

pub fn is_bound_type(ty: IntegralType) -> bool{
    ty == IntegralType::Reference ||
        ty == IntegralType::Object ||
        ty == IntegralType::Enum
}
