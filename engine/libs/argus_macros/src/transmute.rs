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
use proc_macro2::Ident;
use quote::quote;
use syn::__private::quote;
use syn::{parse_macro_input, ItemStruct};

pub(crate) fn ffi_repr(attr: TokenStream, item: TokenStream) -> TokenStream {
    let target = parse_macro_input!(item as ItemStruct);
    let other_name = parse_macro_input!(attr as Ident);

    let target_name = &target.ident;

    let output = quote! {
        #target

        impl From<#other_name> for #target_name {
            fn from(o: #other_name) -> Self {
                unsafe { std::mem::transmute(o) }
            }
        }

        impl From<#target_name> for #other_name {
            fn from(t: #target_name) -> Self {
                unsafe { std::mem::transmute(t) }
            }
        }

        impl From<&#other_name> for &#target_name {
            fn from(o: &#other_name) -> Self {
                unsafe { std::mem::transmute(o) }
            }
        }

        impl From<&mut #other_name> for &mut #target_name {
            fn from(o: &mut #other_name) -> Self {
                unsafe { std::mem::transmute(o) }
            }
        }

        impl From<&#target_name> for &#other_name {
            fn from(t: &#target_name) -> Self {
                unsafe { std::mem::transmute(t) }
            }
        }

        impl From<&mut #target_name> for &mut #other_name {
            fn from(t: &mut #target_name) -> Self {
                unsafe { std::mem::transmute(t) }
            }
        }
    };

    output.into()
}
