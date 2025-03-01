# Argus Code Style

## Style

Argus largely follows the default Rust style with some addendums as listed below.

### Continuation

In the context of multiple consecutive similar statements (such as `const` or `static` declarations), if one or more of
the statements must be split across lines to accomodate the column limit, all such statements should be formatted in the
same way regardless of whether they would fit into fewer lines.

## Code Hygiene

The following are some general guidelines for keeping code as clean and correct as possible.

### Error handling

When a function encounters an exceptional circumstance, the following guidelines should be followed:

- If it doesn't affect further execution of the program and doesn't need to be explicitly handled, log a warning and
  move on.
- If it could be handled in a reasonable way depending on context, use a `Result` to return the error to the caller.
- If it is an unrecoverable condition which could only be caused by incorrect program code (i.e. programmer error),
  `assert!` or `panic!` may be used.

When handling a `Result` returned from a function where the error is unrecoverable, the following guidelines should be
followed:

- If an error can be caused directly by external input of some kind (e.g. by a malformed resource file), the error
  should be logged and the engine should be gracefully shut down. Ideally the error should be bubbled up as far up the
  stack as possible before this occurs.
- If an error cannot reasonably be caused by external input and/or is expected to be the result of programmer error,
  `expect` should be used whenever possible with a succinct explanation of the failed condition.
- If an error condition is expected to be exceedingly rare (e.g. as in a `MutexResult`), `unwrap` may be used.

### Unsafe

Avoid `unsafe` code to the greatest extent possible. If an `unsafe` block _must_ be used, preface the block with a
`// SAFETY` comment with a complete proof as to why the code is sound.

If a function must be made `unsafe`, preface it with a doc comment including a `# Safety` section specifying the exact
preconditions required for the function to be used safely. Additionally, unsafe code within the function should be
prefaced with `// SAFETY` comment as described above wherever feasible.

### Function parameters

Non-primitive function parameters should generally be reference types unless the function would otherwise need to clone
them (at least on the happy path).

When reasonable, `pub` and `pub(crate)` functions should accept `impl Into<String>` and `impl AsRef<str>` parameters
in place of `String` and `&str` when this can be reasonably implemented.

`&mut` parameters should be treated as highly suspect and generally avoided absent a good justification for their use.

### KISS

**KISS** stands for **K**eep **I**t **S**imple, **S**tupid. Always prefer simpler solutions over more complex ones and
readable code over esoteric code unless there is a compelling reason not to (e.g. a significant performance difference).
Always be as explicit as possible (within reason) when building an expression with multiple dependencies. For instance
(courtesy of [grugbrain.dev](https://grugbrain.dev)), an expression like this:

```rust
if contact && !contact.is_active() && (contact.in_group(FAMILY) || contact.in_group(FRIENDS)) {
    // ...
}
```

is much more readable when expressed like this:

```rust
if contact {
    let contact_is_inactive = !contact.is_active();
    let contact_is_family_or_friends = contact.in_group(FAMILY) || contact.in_group(FRIENDS);
    if contact_is_inactive && contact_is_family_or_friends {
        // ...
    }
}
```

The same concept should be used when applying the DRY (**D**on't **R**epeat **Y**ourself) principle. DRY should
generally be followed, but not when the complexity it introduces is disproportionate to how much deduplicating it's
doing. It's better to repeat a few lines of code once or twice than to introduce an unnecessarily complex pattern or to
muddy the call stack beyond recognition.

### Attributes

#### `#[must_use]`

Always use the `#[must_use]` function attribute where appropriate. 
