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
use proc_macro2::{Span as Span2, Span};
use proc_macro2::TokenStream as TokenStream2;
use quote::{format_ident, quote, quote_spanned, ToTokens};
use syn::*;
use core::result::Result;
use argus_scripting_types::*;
use syn::punctuated::Punctuated;
use syn::spanned::Spanned;

use syn::Error as CompileError;
use syn::parse::Parser;

const ATTR_SCRIPT_BIND: &str = "script_bind";
const UNIV_ARG_IGNORE: &str = "ignore";
const STRUCT_ARG_RENAME: &str = "rename";
const STRUCT_ARG_REF_ONLY: &str = "ref_only";
const FN_ARG_ASSOC_TYPE: &str = "assoc_type";
const FN_ARG_RENAME: &str = "rename";

#[derive(Default)]
struct StructAttrArgs {
    name: Option<String>,
    ref_only: bool,
    ref_only_span: Option<Span2>,
}

#[derive(Default)]
struct FnAttrArgs {
    assoc_type: Option<Path>,
    assoc_type_span: Option<Span2>,
    name: Option<String>,
}

#[proc_macro_attribute]
pub fn script_bind(args: TokenStream, input: TokenStream) -> TokenStream {
    let args_parsed = parse_macro_input!(args with Punctuated::<Meta, Token![,]>::parse_terminated);
    let meta_list: Vec<_> = args_parsed.iter().collect();

    // check for ignored tag on attribute
    if meta_list.iter().any(|meta| match meta {
        Meta::Path(path) => {
            path.segments.len() == 1 &&
                path.segments.first().unwrap().ident.to_string() == UNIV_ARG_IGNORE
        }
        _ => false,
    }) {
        // return input token stream unchanged
        return input;
    }

    let item = parse_macro_input!(input as Item);

    let generated = (match item {
        Item::Struct(ref s) => handle_struct(s, meta_list),
        Item::Enum(ref s) => handle_enum(s, meta_list),
        Item::Fn(ref f) => handle_bare_fn(f, meta_list),
        Item::Impl(ref i) => handle_impl(i, meta_list),
        _ => Err(CompileError::new(
            item.span(),
            format!("Attribute '{ATTR_SCRIPT_BIND}' is not allowed here")
        )),
    })
        .unwrap_or_else(|e| { e.into_compile_error() });

    quote!(
        #generated
    ).into()
}

fn handle_struct(item: &ItemStruct, args: Vec<&Meta>) -> Result<TokenStream2, CompileError> {
    let struct_args = parse_struct_args(&args)?;

    let struct_span = item.span();
    let struct_ident = &item.ident;
    let struct_name = struct_args.name.unwrap_or(struct_ident.to_string());

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
                ::argus_scripting_bind::ObjectType {
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

    let wrappable_impl_tokens = quote_spanned! {struct_span=>
        impl Wrappable for #struct_ident {
            type InternalFormat = Self;

            fn wrap_into(self, wrapper: &mut ::argus_scripting_bind::WrappedObject)
            where
                Self: ::argus_scripting_bind::Wrappable<InternalFormat = Self> +
                    ::std::clone::Clone {
                if ::std::mem::size_of::<Self>() > 0 {
                    unsafe { wrapper.store_value::<Self>(self) }
                }
            }

            fn get_required_buffer_size(&self) -> usize
            where
                Self: ::argus_scripting_bind::Wrappable<InternalFormat = Self> +
                    ::std::clone::Clone {
                ::std::mem::size_of::<Self>()
            }

            fn unwrap_as_value(wrapper: &::argus_scripting_bind::WrappedObject) -> Self
            where
                Self: ::argus_scripting_bind::Wrappable<InternalFormat = Self> +
                    ::std::clone::Clone {
                // SAFETY: the implementation provided by this macro guarantees
                //         that Self::InternalFormat == Self
                unsafe { <Self::InternalFormat as Clone>::clone(&*wrapper.get_ptr::<Self>()) }
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
                        format!(
                            "Attribute '{ATTR_SCRIPT_BIND}' cannot be applied to tuple structs"
                        ),
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
                    &[fn () -> ::std::any::TypeId]
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
            #item
            const _: () = {
                use ::argus_scripting_bind::Wrappable;
                #ctor_dtor_proxies_tokens
                #script_bound_impl_tokens
                #wrappable_impl_tokens
                #register_struct_tokens
                #(#field_regs)*
            };
        })
    } else {
        Ok(quote! {
            #item
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
                let attr_name = match nv.path.segments.first() {
                    Some(seg) => {
                        if !seg.arguments.is_empty() {
                            return Err(CompileError::new(
                                seg.arguments.span(),
                                "Unexpected path arguments"
                            ));
                        }

                        seg.ident.to_string()
                    },
                    None => { return Err(CompileError::new(nv.path.span(), "Unexpected path")) }
                };

                if attr_name == STRUCT_ARG_RENAME {
                    args_obj.name = match &nv.value {
                        Expr::Lit(ExprLit { lit: Lit::Str(str_literal), .. }) =>
                            Some(str_literal.value()),
                        _ => {
                            return Err(CompileError::new(
                                nv.value.span(),
                                "Rename argument must be a string literal",
                            ));
                        }
                    };
                } else {
                    return Err(CompileError::new(
                        nv.span(),
                        format!("Invalid attribute argument '{}'", attr_name)
                    ));
                }
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
        #item
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

fn handle_bare_fn(item: &ItemFn, args: Vec<&Meta>) -> Result<TokenStream2, CompileError> {
    let binding_code = gen_fn_binding_code(&item.sig, args, false, None, None)?;
    Ok(quote! {
        #item
        #binding_code
    })
}

fn gen_fn_binding_code(
    sig: &Signature,
    args: Vec<&Meta>,
    is_const: bool,
    assoc_type: Option<&TypePath>,
    attr_span: Option<Span>,
)
    -> Result<TokenStream2, CompileError> {
    let call_site_span = attr_span.unwrap_or(Span::call_site());

    let fn_args = parse_fn_args(args)?;

    let fn_ident = &sig.ident;
    let fn_name = fn_args.name.unwrap_or(fn_ident.to_string());
    let fn_params = &sig.inputs;
    let fn_type = if assoc_type.is_some() {
        if let Some(FnArg::Receiver(_)) = fn_params.first() {
            FunctionType::MemberInstance
        } else {
            FunctionType::MemberStatic
        }
    } else {
        FunctionType::Global
    };

    let param_types: Vec<_> = fn_params.iter()
        .map(|input| {
            match input {
                FnArg::Receiver(self_type) => {
                    if assoc_type.is_none() {
                        return Err(CompileError::new(
                            call_site_span,
                            "Enclosing impl block must be annotated with #[script_bind]"
                        ));
                    }

                    let assoc_type_path =
                        Type::Path(assoc_type.expect("Associated type was missing").clone());
                    match &self_type.reference {
                        Some((and_token, lifetime)) => Ok(Type::Reference(TypeReference {
                            and_token: and_token.clone(),
                            lifetime: lifetime.clone(),
                            mutability: self_type.mutability.clone(),
                            elem: Box::new(assoc_type_path),
                        })),
                        None => Ok(assoc_type_path),
                    }
                },
                FnArg::Typed(ty) => Ok(ty.ty.as_ref().clone()),
            }
        })
        .collect::<Result<_, _>>()?;

    let param_obj_types:
        Vec<(ObjectType, Vec<Type>)> =
        param_types.iter()
            .map(|ty| {
                parse_type(&ty, ValueFlowDirection::FromScript)
            })
            .collect::<Result<_, _>>()?;

    let param_type_serials: Vec<_> = param_obj_types.iter()
        .map(|p| serde_json::to_string(&p.0).expect("Failed to serialize function parameter type"))
        .collect();

    let (ret_obj_type, ret_syn_types) = match &sig.output {
        ReturnType::Default => (EMPTY_TYPE.clone(), vec![]),
        ReturnType::Type(_, ty) => {
            let mut effective_type = None;
            if let Type::Path(path) = ty.as_ref() {
                if path.qself.is_none() && path.path.segments.len() == 1 {
                    let seg = path.path.segments.get(0).expect("Path segment was missing");
                    if seg.ident == "Self" && matches!(seg.arguments, PathArguments::None) {
                        effective_type = Some(
                            Type::Path(assoc_type.expect("Associated type is missing").clone())
                        );
                    }
                }
            }

            parse_type(
                effective_type.as_ref().unwrap_or(ty.as_ref()),
                ValueFlowDirection::ToScript
            )?
        },
    };

    let invocation_span = match &sig.output {
        ReturnType::Default => call_site_span,
        ReturnType::Type(_, ty) => ty.span(),
    };

    let ret_type_serial = serde_json::to_string(&ret_obj_type)
        .expect("Failed to serialize function return type");

    let param_tokens: Vec<_> = param_types.iter().enumerate().map(|(param_index, param_type)| {
        let param_type_span = param_type.span();
        quote_spanned! {param_type_span=>
            params[#param_index].unwrap::<#param_type>()
        }
    }).collect();

    let (proxy_ident, fn_def_ident) = match assoc_type {
        Some(ty) => {
            let ty_ident = &ty.path.segments.last()
                .expect("Associated type had no path segments")
                .ident;
            (
                format_ident!("_{ty_ident}_{fn_ident}_proxied"),
                format_ident!("_{ty_ident}_{fn_ident}_SCRIPT_BIND_DEFINITION"),
            )
        }
        None => {
            (
                format_ident!("_{fn_ident}_proxied"),
                format_ident!("_{fn_ident}_SCRIPT_BIND_DEFINITION"),
            )
        }
    };

    let ret_type_id_getter_tokens: Vec<TokenStream2> = ret_syn_types.into_iter()
        .map(|ty| {
            let ty_span = ty.span();
            quote_spanned! {ty_span=>
                || { ::std::any::TypeId::of::<#ty>() }
            }
        })
        .collect();
    let param_type_id_getters_tokens: Vec<TokenStream2> = param_obj_types.into_iter()
        .map(|(param_obj_type, param_syn_types)| {
            let getters: Vec<_> = param_syn_types.iter()
                .map(|ty| {
                    let ty_span = ty.span();
                    quote_spanned! {ty_span=>
                        || { ::std::any::TypeId::of::<#ty>() }
                    }
                })
                .collect();
            quote! {
                &[#(#getters),*]
            }
        })
        .collect();

    let fn_call = match fn_type {
        FunctionType::MemberInstance | FunctionType::MemberStatic => {
            let ty = assoc_type.expect("Associated type was missing");
            quote_spanned! {invocation_span=>
                <#ty>::#fn_ident(
                    #(#param_tokens),*
                )
            }
        }
        FunctionType::Global => {
            quote_spanned! {invocation_span=>
                #fn_ident(
                    #(#param_tokens),*
                )
            }
        }
        FunctionType::Extension => {
            return Err(CompileError::new(
                call_site_span,
                "Extension function bindings are not yet supported"
            ));
        }
    };

    let wrap_invoke_tokens = quote_spanned! {invocation_span=>
        ::argus_scripting_bind::WrappedObject::wrap
    };

    let assoc_type_getter_tokens = match assoc_type {
        Some(ty) => quote! { Some(|| { ::std::any::TypeId::of::<#ty>() }) },
        None => quote! { None },
    };

    Ok(quote! {
        const _: () = {
            fn #proxy_ident(params: &[::argus_scripting_bind::WrappedObject]) ->
                ::std::result::Result<
                    ::argus_scripting_bind::WrappedObject,
                    ::argus_scripting_bind::ReflectiveArgumentsError
                > {
                Ok(#wrap_invoke_tokens(#fn_call))
            }

            #[::argus_scripting_bind::linkme::distributed_slice(
                ::argus_scripting_bind::BOUND_FUNCTION_DEFS
            )]
            #[linkme(crate = ::argus_scripting_bind::linkme)]
            static #fn_def_ident:
            (
                ::argus_scripting_bind::BoundFunctionInfo,
                &[&[fn () -> ::std::any::TypeId]],
            ) = (
                ::argus_scripting_bind::BoundFunctionInfo {
                    name: #fn_name,
                    ty: #fn_type,
                    is_const: #is_const,
                    param_type_serials: &[
                        #(#param_type_serials),*
                    ],
                    return_type_serial: #ret_type_serial,
                    assoc_type: #assoc_type_getter_tokens,
                    proxy: #proxy_ident as ::argus_scripting_bind::ProxiedNativeFunction,
                },
                &[
                    &[#(#ret_type_id_getter_tokens),*],
                    #(#param_type_id_getters_tokens),*
                ],
            );
        };
    })
}

fn parse_fn_args(meta_list: Vec<&Meta>) -> Result<FnAttrArgs, CompileError> {
    let mut args_obj = FnAttrArgs::default();

    for meta in meta_list {
        match meta {
            Meta::Path(path) => {
                return Err(CompileError::new(path.span(), "Unexpected path"));
            }
            Meta::List(list) => {
                return Err(CompileError::new(list.span(), "Unexpected list"));
            }
            Meta::NameValue(nv) => {
                let attr_name = match nv.path.segments.first() {
                    Some(seg) => {
                        if !seg.arguments.is_empty() {
                            return Err(CompileError::new(
                                seg.arguments.span(),
                                "Unexpected path arguments"
                            ));
                        }

                        seg.ident.to_string()
                    },
                    None => { return Err(CompileError::new(nv.path.span(), "Unexpected path")) }
                };

                if attr_name == STRUCT_ARG_RENAME {
                    args_obj.name = match &nv.value {
                        Expr::Lit(ExprLit { lit: Lit::Str(str_literal), .. }) =>
                            Some(str_literal.value()),
                        _ => {
                            return Err(CompileError::new(
                                nv.value.span(),
                                "Rename argument must be a string literal",
                            ));
                        }
                    };
                } else {
                    return Err(CompileError::new(
                        nv.span(),
                        format!("Invalid attribute argument '{}'", attr_name)
                    ));
                }
            }
        }
    }

    Ok(args_obj)
}

fn handle_impl(impl_block: &ItemImpl, _args: Vec<&Meta>) -> Result<TokenStream2, CompileError> {
    let Type::Path(ty) = impl_block.self_ty.as_ref()
    else {
        return Err(CompileError::new(
            impl_block.self_ty.span(),
            format!("Only primitive and struct types are supported by {ATTR_SCRIPT_BIND}"),
        ));
    };

    let mut fns_to_bind = Vec::<(&ImplItemFn, Meta)>::new();

    let mut transformed_items = Vec::new();
    for item in &impl_block.items {
        let new_item = match item {
            ImplItem::Fn(ref f) => {
                if let Some(pos) = f.attrs.iter().position(|attr| {
                    if attr.path().segments.len() != 1 { return false; }
                    let seg = attr.path().segments.first().expect("First path segment was missing");
                    seg.ident.to_string() == ATTR_SCRIPT_BIND
                }) {
                    fns_to_bind.push((&f, f.attrs[pos].meta.clone()));
                    let mut f_clone = f.clone();
                    // remove script_bind attribute since it's being handled as
                    // part of the impl block
                    let new_list = match &f_clone.attrs[pos].meta {
                        Meta::Path(path) => {
                            MetaList {
                                path: path.clone(),
                                delimiter: MacroDelimiter::Paren(token::Paren::default()),
                                tokens: quote! { ignore },
                            }
                        }
                        Meta::List(list) => {
                            let list_tokens = &list.tokens;
                            MetaList {
                                path: list.path.clone(),
                                delimiter: MacroDelimiter::Paren(token::Paren::default()),
                                tokens: if !list_tokens.is_empty() {
                                    quote! { #list_tokens, ignore }
                                } else {
                                    quote! { ignore }
                                },
                            }
                        }
                        Meta::NameValue(nv) => {
                            return Err(CompileError::new(nv.span(), "Unexpected name/value pair"));
                        }
                    };
                    // avoid getting rid of the macro outright so we don't confuse IDEs
                    f_clone.attrs[pos].meta = Meta::List(new_list);
                    ImplItem::Fn(f_clone)
                } else {
                    item.clone()
                }
            }
            _ => {
                item.clone()
            }
        };
        transformed_items.push(new_item);
    }

    let mut transformed_block = impl_block.clone();
    transformed_block.items = transformed_items;

    let mut output = TokenStream2::new();

    transformed_block.to_tokens(&mut output);

    for (f, args) in fns_to_bind {
        let attr_span = args.span();
        let section_tokens = handle_impl_fn(ty, f, args)?;
        // re-quote the emitted tokens so that any errors in a function appear
        // on that function's attribute (instead of the attribute on the whole
        // impl block)
        let spanned_tokens = quote_spanned! {attr_span=> #section_tokens };
        spanned_tokens.to_tokens(&mut output)
    }

    Ok(output)
}

fn handle_impl_fn(ty: &TypePath, f: &ImplItemFn, args: Meta) -> Result<TokenStream2, CompileError> {
    let fn_attr_span = args.span().clone();
    let meta_list = match args {
        Meta::List(meta_list) => {
            let args_tokens = TokenStream::from(meta_list.tokens);
            let args_parsed =
                Parser::parse(Punctuated::<Meta, Token![,]>::parse_terminated, args_tokens)?;
            args_parsed.into_iter().collect()
        }
        Meta::Path(_) => vec![],
        Meta::NameValue(nv) => {
            return Err(CompileError::new(nv.span(), "Unexpected name/value pair"));
        }
    };
    let meta_list_ref = meta_list.iter().collect();

    let is_const = match f.sig.inputs.first() {
        Some(FnArg::Receiver(recv)) => recv.mutability.is_none(),
        _ => false,
    };

    gen_fn_binding_code(&f.sig, meta_list_ref, is_const, Some(ty), Some(fn_attr_span))
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
            ::argus_scripting_bind::Wrappable {
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
    parse_type_internal(ty, flow_direction, OuterTypeType::None)
}

fn parse_type_internal<'a>(
    ty: &'a Type,
    flow_direction: ValueFlowDirection,
    outer_type: OuterTypeType
)
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

        /*if flow_direction == ValueFlowDirection::FromScript && reference.mutability.is_some() {
            return Err(CompileError::new(
                reference.span(),
                "Mutable reference from script is not allowed in bound symbol"
            ));
        }*/

        let (inner_obj_type, inner_types) =
            parse_type_internal(&reference.elem, flow_direction, OuterTypeType::Reference)?;

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
        parse_type_internal(&inner_type, flow_direction, OuterTypeType::Vec)?;
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
        parse_type_internal(val_type, flow_direction, OuterTypeType::Result)?;
    assert!(resolved_val_types.len() <= 1,
            "Parsing Result type returned too many resolved types for inner value type");
    let (err_obj_type, resolved_err_types) =
        parse_type_internal(err_type, flow_direction, OuterTypeType::Result)?;
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
