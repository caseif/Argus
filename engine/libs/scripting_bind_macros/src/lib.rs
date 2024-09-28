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

use proc_macro::TokenStream;
use proc_macro2::TokenStream as TokenStream2;
use quote::quote;
use syn::*;
use core::result::Result;
use argus_scripting_types::{IntegralType, ObjectType, EMPTY_TYPE, FunctionType};

#[proc_macro_attribute]
pub fn script_bind(_attr: TokenStream, input: TokenStream) -> TokenStream {
    let item = parse_macro_input!(input as Item);

    let generated = match item {
        Item::Struct(ref s) => handle_struct(s),
        Item::Enum(ref s) => handle_enum(s),
        Item::Fn(ref f) => handle_fn(f),
        _ => panic!("Invalid item"),
    };

    quote!(
        #item
        #generated
    ).into()
}

fn handle_struct(item: &ItemStruct) -> TokenStream2 {
    let struct_ident = &item.ident;
    let struct_name = struct_ident.to_string();

    let mut field_regs: Vec<TokenStream2> = Vec::new();

    for field in &item.fields {
        let Visibility::Public(_) = field.vis else { continue; };

        // not sure if this is even possible
        let field_ident = field.ident.as_ref()
            .expect("script_bind attribute cannot be applied to tuple structs");
        let field_name = field_ident.to_string();

        let field_type = &field.ty;

        let field_full_type = parse_type(field_type)
            .expect("Unsupported or invalid field type");
        let field_type_serial = serde_json::to_string(&field_full_type)
            .expect("Failed to serialize field type");

        field_regs.push(quote!(
            ::argus_scripting_bind::inventory::submit! {
                ::argus_scripting_bind::BoundFieldInfo {
                    containing_type: || { ::std::any::TypeId::of::<#struct_ident>() },
                    name: #field_name,
                    type_serial: #field_type_serial,
                    size: ::core::mem::size_of::<#field_type>(),
                    accessor: |obj| unsafe {
                        ::core::ptr::from_mut(&mut (&mut *obj.cast::<#struct_ident>()).#field_ident)
                            .cast()
                    },
                    mutator: |obj, val| unsafe {
                        (&mut *obj.cast::<#struct_ident>()).#field_ident = *val.cast()
                    },
                }
            }
        ));
    }

    quote!(
        ::argus_scripting_bind::inventory::submit! {
            ::argus_scripting_bind::BoundStructInfo {
                name: #struct_name,
                type_id: || { ::std::any::TypeId::of::<#struct_ident>() },
                size: ::core::mem::size_of::<#struct_ident>(),
            }
        }
        #(#field_regs)*
    )
}

fn handle_enum(item: &ItemEnum) -> TokenStream2 {
    let enum_ident = &item.ident;
    let enum_name = enum_ident.to_string();

    let mut last_discrim = quote!(0);
    let mut cur_offset = 0;
    let vals: Vec<TokenStream2> = item.variants.iter().map(|enum_var| {
        if let Fields::Unit = &enum_var.fields {
            let cur_discrim = if let Some((_, explicit_discrim)) = &enum_var.discriminant {
                cur_offset = 0;
                last_discrim = quote!(#explicit_discrim);
                last_discrim.clone()
            } else {
                quote!((#last_discrim) + #cur_offset)
            };
            cur_offset += 1;
            let var_name = enum_var.ident.to_string();
            quote!(
                (#var_name, (|| { (#cur_discrim) as i64 }) as fn() -> i64)
            )
        } else {
            panic!("Enums containing non-unit variants may not be bound");
        }
    }).collect();

    quote!(
        ::argus_scripting_bind::inventory::submit! {
            ::argus_scripting_bind::BoundEnumInfo {
                name: #enum_name,
                type_id: || { ::std::any::TypeId::of::<#enum_ident>() },
                width: ::core::mem::size_of::<#enum_ident>(),
                values: &[#(#vals),*],
            }
        }
    )
}

fn handle_fn(f: &ItemFn) -> TokenStream2 {
    let fn_ident = &f.sig.ident;
    let fn_name = fn_ident.to_string();
    let fn_type = FunctionType::Global; //TODO
    let param_type_serials: Vec<String> = f.sig.inputs.iter()
        .map(|input| {
            serde_json::to_string(
                &parse_type(match input {
                    FnArg::Receiver(r) => r.ty.as_ref(),
                    FnArg::Typed(ty) => ty.ty.as_ref(),
                })
                    .expect("Failed to parse function parameter type")
            )
                .expect("Failed to serialize function parameter type")
        })
        .collect();

    let ret_type_serial = serde_json::to_string(&match &f.sig.output {
        ReturnType::Default => EMPTY_TYPE.clone(),
        ReturnType::Type(_, ty) => {
            let parsed_type = parse_type(ty.as_ref())
                .expect("Failed to parse function return type");
            parsed_type
        },
    })
        .expect("Failed to serialize function return type");

let param_tokens: Vec<_> = param_type_serials.iter().map(|i| {
        quote! { params.remove(0).unwrap() }
    }).collect();

    quote!(
        ::argus_scripting_bind::inventory::submit! {
            ::argus_scripting_bind::BoundFunctionInfo {
                name: #fn_name,
                ty: #fn_type,
                param_type_serials: &[
                    #(#param_type_serials),*
                ],
                return_type_serial: #ret_type_serial,
                proxy: |mut params| {
                    ::argus_scripting_bind::WrappedObject::wrap(
                        #fn_ident(
                            #(#param_tokens),*
                        )
                    )
                },
            }
        }
    )
}

macro_rules! first_some {
    ($h:expr) => ($h);
    ($h:expr, $($t:expr),+ $(,)?) => (
        $h.or_else(|| first_some!($($t),+))
    )
}

fn parse_type(ty: &Type) -> Result<ObjectType, &'static str> {
    if let Type::Path(path) = ty {
        if path.path.segments.len() == 0 {
            return Err("No segments in type path");
        }

        let parsed_opt = first_some!(
            resolve_i8(&path),
            resolve_i16(&path),
            resolve_i32(&path),
            resolve_i64(&path),
            resolve_i128(&path),
            resolve_u8(&path),
            resolve_u16(&path),
            resolve_u32(&path),
            resolve_u64(&path),
            resolve_u128(&path),
            resolve_f32(&path),
            resolve_f64(&path),
            resolve_bool(&path),
            resolve_string(&path),
            resolve_vec(&path),
            resolve_result(&path),
        );

        if parsed_opt.is_none() {
            //
        }

        parsed_opt.map_or_else(|| Err("Type is not supported or is invalid"), |t| Ok(t))
    } else if let Type::Reference(reference) = ty {
        let outer_basic_type = if reference.mutability.is_some() {
            IntegralType::MutReference
        } else {
            IntegralType::ConstReference
        };
        let inner_type = parse_type(&reference.elem)?;
        Ok(ObjectType {
            ty: outer_basic_type,
            size: 0,
            is_const: false,
            is_refable: None,
            primary_type: Some(Box::new(inner_type)),
            secondary_type: None,
            type_id: None,
        })
    } else if let Type::Ptr(pointer) = ty {
        let outer_basic_type = if pointer.mutability.is_some() {
            IntegralType::MutReference
        } else {
            IntegralType::ConstReference
        };
        let inner_type = parse_type(&pointer.elem)?;
        Ok(ObjectType {
            ty: outer_basic_type,
            size: 0,
            is_refable: None,
            is_const: false,
            primary_type: Some(Box::new(inner_type)),
            secondary_type: None,
            type_id: None,
        })
    } else if let Type::Tuple(tuple) = ty {
        if tuple.elems.len() == 0 {
            Ok(ObjectType {
                ty: IntegralType::Empty,
                size: 0,
                is_refable: None,
                is_const: false,
                type_id: None,
                primary_type: None,
                secondary_type: None,
            })
        } else {
            Err("Tuple types are not supported")
        }
    } else {
        Err("Unsupported type")
    }
}

fn match_path(subject: &Path, needle: Vec<&'static str>) -> bool {
    if subject.segments.len() > needle.len() {
        // subject is too long, can't possibly be a subpath
        false
    } else {
        // subject must match last N segments of needle where N is subject segment count
        // in other words, it must be a suffix of needle
        needle[needle.len() - subject.segments.len()..]
            .iter()
            .enumerate()
            .map(|(i, seg)| subject.segments[i].ident.to_string() == *seg)
            .fold(true, |a, b| a && b)
    }
}

macro_rules! resolve_basic_type {
    ($func:ident, $ty:ident, $enum_var:ident) => (
        fn $func(path: &TypePath) -> Option<ObjectType> {
            (&path.path.segments[0].ident == stringify!($ty)).then_some(
                ObjectType {
                    ty: IntegralType::$enum_var,
                    is_const: false,
                    is_refable: None,
                    size: 0,
                    type_id: None,
                    primary_type: None,
                    secondary_type: None,
                }
            )
        }
    )
}

resolve_basic_type!(resolve_i8, i8, Int8);
resolve_basic_type!(resolve_i16, i16, Int16);
resolve_basic_type!(resolve_i32, i32, Int32);
resolve_basic_type!(resolve_i64, i64, Int64);
resolve_basic_type!(resolve_i128, i128, Int128);
resolve_basic_type!(resolve_u8, u8, Uint8);
resolve_basic_type!(resolve_u16, u16, Uint16);
resolve_basic_type!(resolve_u32, u32, Uint32);
resolve_basic_type!(resolve_u64, u64, Uint64);
resolve_basic_type!(resolve_u128, u128, Uint128);
resolve_basic_type!(resolve_f32, f32, Float32);
resolve_basic_type!(resolve_f64, f64, Float64);
resolve_basic_type!(resolve_bool, bool, Boolean);

fn resolve_string(ty: &TypePath) -> Option<ObjectType> {
    if ty.path.segments[0].ident != "str" && !match_path(&ty.path, vec!["alloc", "String"]) {
        return None;
    }

    Some(ObjectType {
        ty: IntegralType::String,
        is_const: false,
        is_refable: None,
        size: 0,
        type_id: None,
        primary_type: None,
        secondary_type: None,
    })
}

fn resolve_vec(ty: &TypePath) -> Option<ObjectType> {
    if !match_path(&ty.path, vec!["std", "collections", "Vec"]) {
        return None;
    }

    let PathArguments::AngleBracketed(el_ty) =
        &ty.path.segments[ty.path.segments.len() - 1].arguments
    else { panic!("Invalid Vec argument syntax"); };
    let GenericArgument::Type(inner_ty) = el_ty.args.first().expect("Invalid Vec argument syntax")
    else { panic!("Invalid Vec argument syntax"); };

    Some(ObjectType {
        ty: IntegralType::Vector,
        size: 0,
        is_refable: None,
        is_const: false,
        type_id: None,
        primary_type: Some(Box::new(parse_type(inner_ty).unwrap())),
        secondary_type: None,
    })
}

fn resolve_result(ty: &TypePath) -> Option<ObjectType> {
    if !match_path(&ty.path, vec!["core", "Result"]) {
        return None;
    }

    let PathArguments::AngleBracketed(el_ty) =
        &ty.path.segments[ty.path.segments.len() - 1].arguments
    else { panic!("Invalid Result argument syntax"); };

    let mut it = el_ty.args.iter();

    let GenericArgument::Type(val_ty) = it.next().expect("Invalid Result argument syntax")
    else { panic!("Invalid Result argument syntax"); };
    let GenericArgument::Type(err_ty) = it.next().expect("Invalid Result argument syntax")
    else { panic!("Invalid Result argument syntax"); };

    Some(ObjectType {
        ty: IntegralType::Result,
        size: 0,
        is_const: false,
        is_refable: None,
        primary_type: Some(Box::new(parse_type(val_ty).unwrap())),
        secondary_type: Some(Box::new(parse_type(err_ty).unwrap())),
        type_id: None,
    })
}
