use proc_macro::TokenStream;
use proc_macro2::{Ident, Span};
use quote::{format_ident, quote};
use syn::{parse_macro_input, parse_quote, ItemFn, LitStr, Path};

const ARG_CRATE: &str = "crate";
const ARG_ID: &str = "id";
const ARG_DEPENDS_ON: &str = "depends";

pub(crate) fn register_module(args: TokenStream, input: TokenStream) -> TokenStream {
    let target = parse_macro_input!(input as ItemFn);

    let mut core_path_custom: Option<Path> = Some(parse_quote! { ::argus_core });
    let mut args_obj = RegisterArgs::default();
    let args_parser = syn::meta::parser(|meta| {
        if meta.path.is_ident(ARG_CRATE) {
            core_path_custom = Some(meta.value()?.parse()?);
            Ok(())
        } else if meta.path.is_ident(ARG_ID) {
            args_obj.id = Some(meta.value()?.parse::<LitStr>()?);
            Ok(())
        } else if meta.path.is_ident(ARG_DEPENDS_ON) {
            meta.parse_nested_meta(|depends_meta| {
                args_obj.depends.push(depends_meta.path.require_ident()?.clone());
                Ok(())
            })
        } else {
            Err(meta.error("Unsupported registration property"))
        }
    });
    parse_macro_input!(args with args_parser);

    let args_obj = args_obj;

    if args_obj.id.is_none() {
        return syn::Error::new(Span::call_site(), "Module name is required")
            .into_compile_error()
            .into();
    }

    let core_path = core_path_custom.unwrap_or(parse_quote! { ::core_rs });

    let module_id = args_obj.id;
    let module_depends_on: Vec<_> = args_obj.depends.into_iter()
        .map(|dep| dep.to_string())
        .collect();

    let target_ident = &target.sig.ident;

    let module_def_ident = format_ident!("_argus_module_{target_ident}_registration");

    let registration = quote! {
        const _: () = {
            #[#core_path::internal::reexport::linkme::distributed_slice(
                #core_path::internal::register::REGISTERED_MODULES
            )]
            #[linkme(crate = #core_path::internal::reexport::linkme)]
            static #module_def_ident: #core_path::internal::register::ModuleRegistration =
            #core_path::internal::register::ModuleRegistration {
                id: #module_id,
                depends_on: &[#(#module_depends_on),*],
                entry_point: #target_ident,
            };
        };
    };

    quote!(
        #registration

        #target
    ).into()
}

#[derive(Clone, Default)]
struct RegisterArgs {
    id: Option<LitStr>,
    depends: Vec<Ident>,
}
