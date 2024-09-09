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
use syn::{parse_macro_input, Fields, Item, ItemEnum, ItemFn, ItemStruct};

#[proc_macro_attribute]
pub fn script_bind(attr: TokenStream, input: TokenStream) -> TokenStream {
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
    quote!(
        ::argus_scripting_bind::inventory::submit! {
            ::argus_scripting_bind::BoundStructInfo::new(
                #struct_name,
                || { std::any::TypeId::of::<#struct_ident>() },
                size_of::<#struct_ident>(),
            )
        }
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
            ::argus_scripting_bind::BoundEnumInfo::new(
                #enum_name,
                || { std::any::TypeId::of::<#enum_ident>() },
                size_of::<#enum_ident>(),
                &[#(#vals),*],
            )
        }
    )
}

fn handle_fn(f: &ItemFn) -> TokenStream2 {
    quote!()
}
