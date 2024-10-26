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
use proc_macro2::Span as Span2;
use proc_macro2::TokenStream as TokenStream2;
use quote::{format_ident, quote, quote_spanned};
use syn::*;
use core::result::Result;
use argus_scripting_types::*;
use syn::punctuated::Punctuated;
use syn::spanned::Spanned;

use syn::Error as CompileError;

const STRUCT_ARG_REF_ONLY: &'static str = "ref_only";

#[derive(Default)]
struct StructAttrArgs {
    ref_only: bool,
    ref_only_span: Option<Span2>,
}

#[proc_macro_attribute]
pub fn script_bind(args: TokenStream, input: TokenStream) -> TokenStream {
    let args_parsed = parse_macro_input!(args with Punctuated::<Meta, Token![,]>::parse_terminated);
    let meta_list = args_parsed.iter().collect();

    let item = parse_macro_input!(input as Item);

    let generated = (match item {
        Item::Struct(ref s) => handle_struct(s, meta_list),
        Item::Enum(ref s) => handle_enum(s, meta_list),
        Item::Fn(ref f) => handle_fn(f, meta_list),
        _ => Err(CompileError::new(item.span(), "Attribute 'script_bound' is not allowed here")),
    })
        .unwrap_or_else(|e| { e.into_compile_error() });

    quote!(
        #item
        #generated
    ).into()
}

fn handle_struct(item: &ItemStruct, args: Vec<&Meta>) -> Result<TokenStream2, CompileError> {
    let struct_args = parse_struct_args(&args)?;

    let struct_span = item.span();
    let struct_ident = &item.ident;
    let struct_name = struct_ident.to_string();

    let ctor_dtor_proxies_tokens = quote! {
        unsafe extern "C" fn _copy_ctor(dst: *mut (), src: *const ()) {
            ::std::ptr::write(
                ::std::ptr::addr_of_mut!(*dst.cast::<#struct_ident>()),
                ::std::clone::Clone::clone(&*src.cast::<#struct_ident>()),
            );
        }

        unsafe extern "C" fn _move_ctor(dst: *mut (), src: *mut ()) {
            _copy_ctor(dst, src);
            _dtor(src);
        }

        unsafe extern "C" fn _dtor(target: *mut ()) {
            ::std::ptr::drop_in_place(&mut *target.cast::<#struct_ident>());
        }
    };

    let allow_clone = !struct_args.ref_only;

    let struct_def_ctor_dtor_field_init_tokens = if allow_clone {
        quote_spanned! {struct_span=>
            copy_ctor: Some(_copy_ctor),
            move_ctor: Some(_move_ctor),
            dtor: Some(_dtor),
        }
    } else {
        quote_spanned! {struct_span=>
            copy_ctor: None,
            move_ctor: None,
            dtor: None,
        }
    };

    let obj_type_ctor_dtor_field_init_tokens = if allow_clone {
        quote_spanned! {struct_span=>
            copy_ctor: Some(::argus_scripting_bind::CopyCtorWrapper::of(_copy_ctor)),
            move_ctor: Some(::argus_scripting_bind::MoveCtorWrapper::of(_move_ctor)),
            dtor: Some(::argus_scripting_bind::DtorWrapper::of(_dtor)),
        }
    } else {
        quote_spanned! {struct_span=>
            copy_ctor: None,
            move_ctor: None,
            dtor: None,
        }
    };

    let script_bound_impl_tokens = quote_spanned! {struct_span=>
        impl ::argus_scripting_bind::ScriptBound for #struct_ident {
            fn get_object_type() -> ::argus_scripting_bind::ObjectType {
                argus_scripting_bind::ObjectType {
                    ty: ::argus_scripting_bind::IntegralType::Object,
                    size: size_of::<#struct_ident>(),
                    is_const: false,
                    is_refable: None,
                    is_refable_getter: None,
                    type_id: Some(::std::any::TypeId::of::<#struct_ident>()),
                    type_name: Some(#struct_name.to_string()),
                    primary_type: None,
                    secondary_type: None,
                    #obj_type_ctor_dtor_field_init_tokens
                }
            }
        }
    };

    let wrappable_impls_tokens = quote_spanned! {struct_span=>
        impl<'a> ::argus_scripting_bind::Wrappable<'a> for #struct_ident {
            type InternalFormat = #struct_ident;

            fn wrap_into(self, wrapper: &mut argus_scripting_bind::WrappedObject) {
                assert_eq!(
                    wrapper.ty.ty,
                    argus_scripting_bind::IntegralType::Object,
                    "Wrong object type"
                );

                unsafe { wrapper.store_value::<Self>(self) }
            }
        }

        impl<'a> ::argus_scripting_bind::DirectWrappable<'a> for #struct_ident
            where #struct_ident : ::argus_scripting_bind::Wrappable<'a> {
            fn unwrap_from(wrapper: &'a argus_scripting_bind::WrappedObject) -> &'a Self {
                assert_eq!(
                    wrapper.ty.ty,
                    ::argus_scripting_bind::IntegralType::Object,
                    "Wrong object type"
                );

                unsafe { &*wrapper.get_ptr::<&Self>().cast::<Self>() }
            }
            fn unwrap_from_mut(wrapper: &'a mut argus_scripting_bind::WrappedObject) ->
                &'a mut Self {
                assert_eq!(
                    wrapper.ty.ty,
                    ::argus_scripting_bind::IntegralType::Object,
                    "Wrong object type"
                );

                unsafe { &mut *wrapper.get_mut_ptr::<&Self>().cast::<Self>() }
            }
        }
    };

    let struct_def_ident = format_ident!("_{struct_ident}_SCRIPT_BIND_DEFINITION");
    let register_struct_tokens = quote_spanned! {struct_span=>
        #[::argus_scripting_bind::linkme::distributed_slice(
            ::argus_scripting_bind::BOUND_STRUCT_DEFS
        )]
        #[linkme(crate = ::argus_scripting_bind::linkme)]
        static #struct_def_ident: ::argus_scripting_bind::BoundStructInfo =
            ::argus_scripting_bind::BoundStructInfo {
                name: #struct_name,
                type_id: || { ::std::any::TypeId::of::<#struct_ident>() },
                size: size_of::<#struct_ident>(),
                #struct_def_ctor_dtor_field_init_tokens
            };
    };

    let mut field_regs: Vec<TokenStream2> = Vec::new();

    for field in &item.fields {
        let Visibility::Public(_) = field.vis else { continue; };

        let field_span = field.span();
        let field_ident = {
            match field.ident.as_ref() {
                Some(ident) => ident,
                None => {
                    return Err(CompileError::new(
                        field.span(),
                        "script_bind attribute cannot be applied to tuple structs",
                    ));
                }
            }
        };
        let field_name = field_ident.to_string();
        let field_type = &field.ty;

        let (field_obj_type, field_type_resolved_types) =
            parse_type(&field.ty, ValueFlowDirection::ToScript)?;
        let field_type_serial =
            serde_json::to_string(&field_obj_type).expect("Failed to serialize field type");

        let type_id_getter_tokens: Vec<_> = field_type_resolved_types
            .iter()
            .map(|ty| {
                quote! {
                    || { ::std::any::TypeId::of::<#ty>() }
                }
            })
            .collect();

        let field_type_assertion = build_wrappable_assertion(&field.ty);

        let field_def_ident =
            format_ident!("_{struct_ident}_FIELD_{field_ident}_SCRIPT_BIND_DEFINITION");

        let wrap_field_val_tokens = if field_obj_type.ty == IntegralType::Object {
            quote! {
                ::argus_scripting_bind::WrappedObject::wrap(
                    &((&*inst.cast::<#struct_ident>()).#field_ident)
                )
            }
        } else {
            quote! {
                ::argus_scripting_bind::WrappedObject::wrap(
                    (&*inst.cast::<#struct_ident>()).#field_ident
                )
            }
        };

        field_regs.push(quote_spanned! {field_span=>
            const _: () = {
                #field_type_assertion

                #[::argus_scripting_bind::linkme::distributed_slice(
                    ::argus_scripting_bind::BOUND_FIELD_DEFS
                )]
                #[linkme(crate = ::argus_scripting_bind::linkme)]
                static #field_def_ident:
                (
                    ::argus_scripting_bind::BoundFieldInfo,
                    &'static [fn () -> ::std::any::TypeId]
                ) =
                    (
                        ::argus_scripting_bind::BoundFieldInfo {
                            containing_type: || { ::std::any::TypeId::of::<#struct_ident>() },
                            name: #field_name,
                            type_serial: #field_type_serial,
                            size: size_of::<#field_type>(),
                            accessor: |inst| unsafe {
                                #wrap_field_val_tokens
                            },
                            mutator: |inst, val| unsafe {
                                (&mut *inst.cast::<#struct_ident>()).#field_ident =
                                    (*val.cast::<#field_type>())
                            },
                        },
                        &[#(#type_id_getter_tokens),*],
                    );
            };
        });
    }

    if allow_clone {
        Ok(quote! {
            const _: () = {
                use ::argus_scripting_bind::Wrappable;
                #ctor_dtor_proxies_tokens
                #script_bound_impl_tokens
                #wrappable_impls_tokens
                #register_struct_tokens
                #(#field_regs)*
            };
        })
    } else {
        Ok(quote! {
            const _: () = {
                use ::argus_scripting_bind::Wrappable;
                #script_bound_impl_tokens
                #register_struct_tokens
                #(#field_regs)*
            };
        })
    }
}

fn parse_struct_args(meta_list: &Vec<&Meta>) -> Result<StructAttrArgs, CompileError> {
    let mut args_obj = StructAttrArgs::default();

    for meta in meta_list {
        match meta {
            Meta::Path(path) => {
                let attr_name = match path.segments.first() {
                    Some(seg) => {
                        if !seg.arguments.is_empty() {
                            return Err(CompileError::new(
                                seg.arguments.span(),
                                "Unexpected path arguments"
                            ));
                        }

                        seg.ident.to_string()
                    },
                    None => { return Err(CompileError::new(path.span(), "Unexpected path")) }
                };

                if attr_name == STRUCT_ARG_REF_ONLY {
                    args_obj.ref_only = true;
                    args_obj.ref_only_span = Some(path.span());
                } else {
                    return Err(CompileError::new(
                        path.span(),
                        format!("Invalid attribute argument '{}'", attr_name)
                    ));
                }
            }
            Meta::List(list) => {
                return Err(CompileError::new(list.span(), "Unexpected list"));
            }
            Meta::NameValue(nv) => {
                return Err(CompileError::new(nv.span(), "Unexpected name-value"));
            }
        }
    }

    Ok(args_obj)
}

fn handle_enum(item: &ItemEnum, args: Vec<&Meta>) -> Result<TokenStream2, CompileError> {
    let enum_ident = &item.ident;
    let enum_name = enum_ident.to_string();

    let mut last_discrim = quote!(0);
    let mut cur_offset = 0;
    let vals: Vec<TokenStream2> = item.variants.iter()
        .map(|enum_var| {
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
                Ok(quote! {
                    (#var_name, (|| { (#cur_discrim) as i64 }) as fn() -> i64)
                })
            } else {
                return Err(CompileError::new(
                    enum_var.span(),
                    "Enums containing non-unit variants may not be bound",
                ));
            }
        })
        .collect::<Result<_, _>>()?;

    let enum_def_ident = format_ident!("_{enum_ident}_SCRIPT_BIND_DEFINITION");
    Ok(quote! {
        const _: () = {
            #[::argus_scripting_bind::linkme::distributed_slice(
                ::argus_scripting_bind::BOUND_ENUM_DEFS
            )]
            #[linkme(crate = ::argus_scripting_bind::linkme)]
            static #enum_def_ident: ::argus_scripting_bind::BoundEnumInfo =
                ::argus_scripting_bind::BoundEnumInfo {
                    name: #enum_name,
                    type_id: || { ::std::any::TypeId::of::<#enum_ident>() },
                    width: size_of::<#enum_ident>(),
                    values: &[#(#vals),*],
                };
        };
    })
}

fn handle_fn(f: &ItemFn, args: Vec<&Meta>) -> Result<TokenStream2, CompileError> {
    let fn_ident = &f.sig.ident;
    let fn_name = fn_ident.to_string();
    let fn_type = FunctionType::Global; //TODO
    let fn_args = &f.sig.inputs;

    let param_types: Vec<_> = fn_args.iter()
        .map(|input| {
            match input {
                FnArg::Receiver(r) => r.ty.as_ref(),
                FnArg::Typed(ty) => ty.ty.as_ref(),
            }
        })
        .collect();

    let param_types:
        Vec<(ObjectType, Vec<Type>)> =
        param_types.iter()
            .map(|ty| {
                parse_type(ty, ValueFlowDirection::FromScript)
            })
            .collect::<Result<_, _>>()?;

    let param_type_serials: Vec<_> = param_types.iter()
        .map(|p| serde_json::to_string(&p.0).expect("Failed to serialize function parameter type"))
        .collect();

    let (ret_obj_type, ret_syn_types) = match &f.sig.output {
        ReturnType::Default => (EMPTY_TYPE.clone(), vec![]),
        ReturnType::Type(_, ty) => parse_type(ty.as_ref(), ValueFlowDirection::ToScript)?,
    };

    let invocation_span = match &f.sig.output {
        ReturnType::Default => Span2::call_site(),
        ReturnType::Type(_, ty) => ty.span(),
    };

    let ret_type_serial = serde_json::to_string(&ret_obj_type)
        .expect("Failed to serialize function return type");

    let param_tokens: Vec<_> = fn_args.iter().enumerate().map(|(param_index, fn_arg)| {
        let param_type = match fn_arg {
            FnArg::Receiver(ty) => ty.ty.as_ref(),
            FnArg::Typed(ty) => ty.ty.as_ref(),
        };
        let param_type_span = param_type.span();
        quote_spanned! {param_type_span=>
            <#param_type>::unwrap_as_value(&params[#param_index])
        }
    }).collect();

    let proxy_ident = format_ident!("_{fn_ident}_proxied");
    let fn_def_ident = format_ident!("_{fn_ident}_SCRIPT_BIND_DEFINITION");

    let wrap_invoke_tokens = quote_spanned! {invocation_span=>
        ::argus_scripting_bind::WrappedObject::wrap
    };

    let ret_type_id_getter_tokens: Vec<TokenStream2> = ret_syn_types.into_iter()
        .map(|ty| {
            let ty_span = ty.span();
            quote_spanned! {ty_span=>
                || { ::std::any::TypeId::of::<#ty>() }
            }
        })
        .collect();
    let param_type_id_getters_tokens: Vec<TokenStream2> = param_types.into_iter()
        .map(|(param_obj_type, param_syn_types)| {
            let getters: Vec<_> = param_syn_types.iter()
                .map(|ty| {
                    let ty_span = ty.span();
                    quote_spanned! {ty_span=>
                        Some(|| { ::std::any::TypeId::of::<#ty>() })
                    }
                })
                .collect();
            quote! {
                &[#(#getters),*]
            }
        })
        .collect();

    Ok(quote! {
        const _: () = {
            use ::argus_scripting_bind::{DirectWrappable, IndirectWrappable};

            fn #proxy_ident(params: &[::argus_scripting_bind::WrappedObject]) ->
                ::std::result::Result<
                    ::argus_scripting_bind::WrappedObject,
                    ::argus_scripting_bind::ReflectiveArgumentsError
                > {
                Ok(#wrap_invoke_tokens(
                    #fn_ident(
                        #(#param_tokens),*
                    )
                ))
            }

            #[::argus_scripting_bind::linkme::distributed_slice(
                ::argus_scripting_bind::BOUND_FUNCTION_DEFS
            )]
            #[linkme(crate = ::argus_scripting_bind::linkme)]
            static #fn_def_ident:
            (
                ::argus_scripting_bind::BoundFunctionInfo,
                &'static [&'static [fn () -> ::std::any::TypeId]],
            ) = (
                ::argus_scripting_bind::BoundFunctionInfo {
                    name: #fn_name,
                    ty: #fn_type,
                    param_type_serials: &[
                        #(#param_type_serials),*
                    ],
                    return_type_serial: #ret_type_serial,
                    proxy: &(#proxy_ident as ::argus_scripting_bind::ProxiedNativeFunction),
                },
                &[
                    &[#(#ret_type_id_getter_tokens),*],
                    #(#param_type_id_getters_tokens),*
                ],
            );
        };
    })
}

fn build_wrappable_assertion(ty: &Type) -> TokenStream2 {
    let ty_span = ty.span();
    let def_lifetime = Lifetime::new("'_assertion_lifetime", ty_span);

    let lifetime = match ty {
        Type::Reference(ref_type) => {
            ref_type.lifetime.as_ref().unwrap_or(&def_lifetime)
        }
        _ => &def_lifetime
    };

    let transformed_type: Type = match ty {
        Type::Reference(ref_type) => {
            let mut new_type = ref_type.clone();
            new_type.lifetime = Some(lifetime.clone());
            Type::Reference(new_type)
        }
        _ => {
            ty.clone()
        }
    };
    quote_spanned! {ty_span=>
        struct _AssertWrappable<#lifetime>
        where #transformed_type:
            ::argus_scripting_bind::ScriptBound +
            ::argus_scripting_bind::Wrappable<#lifetime> {
            _marker: ::std::marker::PhantomData<&#lifetime ()>,
        }
    }
}

macro_rules! first_some {
    ($h:expr) => ($h);
    ($h:expr, $($t:expr),+ $(,)?) => (
        $h.or_else(|| first_some!($($t),+))
    )
}

#[derive(Clone, Copy, PartialEq, PartialOrd)]
enum ValueFlowDirection {
    ToScript,
    FromScript,
}

#[derive(Clone, Copy, PartialEq, PartialOrd)]
enum OuterTypeType {
    None,
    Reference,
    MutReference,
    Vec,
    Result,
}

fn parse_type<'a>(ty: &'a Type, flow_direction: ValueFlowDirection)
    -> Result<(ObjectType, Vec<Type>), CompileError> {
    parse_type_impl(ty, flow_direction, OuterTypeType::None)
}

fn parse_type_impl<'a>(ty: &'a Type, flow_direction: ValueFlowDirection, outer_type: OuterTypeType)
    -> Result<(ObjectType, Vec<Type>), CompileError> {
    if let Type::Path(path) = ty {
        if path.path.segments.len() == 0 {
            return Err(CompileError::new(path.span(), "Type path does not contain any segments"));
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
        );

        if let Some(res) = parsed_opt {
            return Ok((res, vec![]));
        }

        if let Some(res) = resolve_vec(&path, flow_direction)? {
            if outer_type != OuterTypeType::None &&
                outer_type != OuterTypeType::Reference &&
                outer_type != OuterTypeType::MutReference {
                return Err(CompileError::new(path.span(), "Vec may not be used as an inner type"));
            }

            let resolved_inner_type = res.1.into_iter().collect();
            return Ok((res.0, resolved_inner_type));
        }

        if let Some(res) = resolve_result(&path, flow_direction)? {
            if outer_type != OuterTypeType::None {
                if outer_type == OuterTypeType::Reference ||
                    outer_type == OuterTypeType::MutReference {
                    return Err(CompileError::new(
                        path.span(),
                        "Result may not be passed as references in bound symbol"
                    ));
                } else {
                    return Err(CompileError::new(
                        path.span(), "Result may not be used as an inner type"
                    ));
                }
            }

            let resolved_inner_types = [res.1, res.2].into_iter()
                .filter_map(|opt| opt)
                .collect();
            return Ok((res.0, resolved_inner_types));
        }

        // treat it as a generic bound type

        let type_name = path.path.segments[path.path.segments.len() - 1].ident.to_string();

        Ok((
            ObjectType {
                ty: IntegralType::Object,
                size: 0,
                is_const: false,
                is_refable: None,
                is_refable_getter: None,
                type_id: None,
                type_name: Some(type_name),
                primary_type: None,
                secondary_type: None,
                copy_ctor: None,
                move_ctor: None,
                dtor: None,
            },
           vec![ty.clone()],
       ))
    } else if let Type::Reference(reference) = ty {
        if let Type::Path(path) = reference.elem.as_ref() {
            if let Some(res) = resolve_string(path) {
                return Ok((res, vec![]));
            }
        }

        if outer_type != OuterTypeType::None {
            return Err(CompileError::new(
                reference.span(),
                "Non-top level reference is not allowed in bound symbol"
            ));
        }

        if flow_direction == ValueFlowDirection::FromScript && reference.mutability.is_some() {
            return Err(CompileError::new(
                reference.span(),
                "Mutable reference from script is not allowed in bound symbol"
            ));
        }

        let (inner_obj_type, inner_types) =
            parse_type_impl(&reference.elem, flow_direction, OuterTypeType::Reference)?;

        if inner_obj_type.ty != IntegralType::Object {
            return Err(CompileError::new(
                reference.span(),
                "Non-struct reference type is not allowed in bound symbol"
            ));
        }

        Ok((
            ObjectType {
                ty: IntegralType::Reference,
                size: size_of::<*const ()>(),
                is_const: false,
                is_refable: None,
                is_refable_getter: None,
                type_id: None,
                type_name: None,
                primary_type: Some(Box::new(inner_obj_type)),
                secondary_type: None,
                copy_ctor: None,
                move_ctor: None,
                dtor: None,
            },
            inner_types,
        ))
    } else if let Type::Ptr(_) = ty {
        Err(CompileError::new(ty.span(), "Pointer is not allowed in bound symbol"))
    } else if let Type::Tuple(tuple) = ty {
        if tuple.elems.len() == 0 {
            Ok((
                ObjectType {
                    ty: IntegralType::Empty,
                    size: 0,
                    is_const: false,
                    is_refable: None,
                    is_refable_getter: None,
                    type_id: None,
                    type_name: None,
                    primary_type: None,
                    secondary_type: None,
                    copy_ctor: None,
                    move_ctor: None,
                    dtor: None,
                },
                vec![],
            ))
        } else {
            Err(CompileError::new(tuple.span(), "Tuple types are not supported"))
        }
    } else {
        Err(CompileError::new(ty.span(), "Unsupported type"))
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
                    size: size_of::<$ty>(),
                    is_const: false,
                    is_refable: None,
                    is_refable_getter: None,
                    type_id: None,
                    type_name: None,
                    primary_type: None,
                    secondary_type: None,
                    copy_ctor: None,
                    move_ctor: None,
                    dtor: None,
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
        size: 0,
        is_const: false,
        is_refable: None,
        is_refable_getter: None,
        type_id: None,
        type_name: None,
        primary_type: None,
        secondary_type: None,
        copy_ctor: Some(CopyCtorWrapper::of(copy_string)),
        move_ctor: Some(MoveCtorWrapper::of(move_string)),
        dtor: Some(DtorWrapper::of(drop_string)),
    })
}

fn resolve_vec(ty: &TypePath, flow_direction: ValueFlowDirection)
    -> Result<Option<(ObjectType, Option<Type>)>, CompileError> {
    if !match_path(&ty.path, vec!["std", "collections", "Vec"]) {
        return Ok(None);
    }

    let PathArguments::AngleBracketed(el_ty) =
        &ty.path.segments[ty.path.segments.len() - 1].arguments
    else { return Err(CompileError::new(ty.span(), "Invalid Vec argument syntax")); };
    let GenericArgument::Type(inner_type) = (match el_ty.args.first() {
        Some(ty) => ty,
        None => { return Err(CompileError::new(ty.span(), "Invalid Vec argument syntax")); }
    })
    else { return Err(CompileError::new(ty.span(), "Invalid Vec argument syntax")); };

    let (inner_obj_type, resolved_inner_types) =
        parse_type_impl(&inner_type, flow_direction, OuterTypeType::Vec)?;
    assert!(resolved_inner_types.len() <= 1,
            "Parsing Vec type returned too many resolved types for inner type");

    Ok(Some((
        ObjectType {
            ty: IntegralType::Vec,
            size: 0,
            is_const: false,
            is_refable: None,
            is_refable_getter: None,
            type_id: None,
            type_name: None,
            primary_type: Some(Box::new(inner_obj_type)),
            secondary_type: None,
            copy_ctor: None,
            move_ctor: None,
            dtor: None,
        },
        resolved_inner_types.first().cloned(),
    )))
}

fn resolve_result<'a>(ty: &'a TypePath, flow_direction: ValueFlowDirection)
    -> Result<Option<(ObjectType, Option<Type>, Option<Type>)>, CompileError> {
    if !match_path(&ty.path, vec!["core", "Result"]) {
        return Ok(None);
    }

    let PathArguments::AngleBracketed(el_ty) =
        &ty.path.segments[ty.path.segments.len() - 1].arguments
    else { return Err(CompileError::new(ty.span(), "Invalid Result argument syntax")); };

    let mut it = el_ty.args.iter();

    let GenericArgument::Type(val_type) = (match it.next() {
        Some(ty) => ty,
        None => { return Err(CompileError::new(ty.span(), "Invalid Result argument syntax")); }
    })
    else { return Err(CompileError::new(ty.span(), "Invalid Result argument syntax")); };
    let GenericArgument::Type(err_type) = (match it.next() {
        Some(ty) => ty,
        None => { return Err(CompileError::new(ty.span(), "Invalid Result argument syntax")); }
    })
    else { return Err(CompileError::new(ty.span(), "Invalid Result argument syntax")); };

    let (val_obj_type, resolved_val_types) =
        parse_type_impl(val_type, flow_direction, OuterTypeType::Result)?;
    assert!(resolved_val_types.len() <= 1,
            "Parsing Result type returned too many resolved types for inner value type");
    let (err_obj_type, resolved_err_types) =
        parse_type_impl(err_type, flow_direction, OuterTypeType::Result)?;
    assert!(resolved_err_types.len() <= 1,
            "Parsing Result type returned too many resolved types for inner error type");

    Ok(Some((
        ObjectType {
            ty: IntegralType::Result,
            size: 0,
            is_const: false,
            is_refable: None,
            is_refable_getter: None,
            type_id: None,
            type_name: None,
            primary_type: Some(Box::new(val_obj_type)),
            secondary_type: Some(Box::new(err_obj_type)),
            copy_ctor: None,
            move_ctor: None,
            dtor: None,
        },
        resolved_val_types.first().cloned(),
        resolved_err_types.first().cloned(),
    )))
}
