# Argus Code Style

This file contains a WIP set of code style rules that should be followed across the project. Parts of the codebase
diverge from these rules, although all new code should adhere to them and existing code will be updated over time to
reflect them as well.

These rules are largely based on the [Google C++ Style Guide](https://en.wikipedia.org/wiki/Indentation_style#K&R) and
aren't exhaustive, so Google style should be followed for everything not covered here.

## Whitespace

Spaces should used for indentation. Unless otherwise noted, one indent level is four spaces.

### Continuation

If a statement does not fit on a single line, an indentation of eight spaces should be used in its continuation on the
following line. Any subsequent continuations within the same expression scope should have the same indentation level,
but a continuation of any nested expression should have an additional continuation indent. As an example:

```c++
// both newlines are within the same scope
int i = chocolate(42)
        + cheese(42)
        + guava(42);

// call to guava is inside the call to cheese
// so it must be indented further
int j = chocolate(42)
        + cheese(42
                + guava(42));
```

### Newlines

In all cases, there should never be two or more consecutive newlines in a file.

Function 

A single newline should be placed at the end of each file.

### Blocks

All blocks should place the opening brace on the same line as the block declaration and precede it with a space. The
closing brace should have the same indentation as the block declaration. The body should be indented one level from the
declaration.

### Parenthetical expressions

Expressions surrounded by parentheses should have no whitespace after the opening parenthesis and no whitespace before
the closing parenthesis.

### Include Directives

Include directives should be placed all together with a newline at both the beginning and end of the include list.
Related includes should be relegated to their own sections with a newline between each section.

### Preprocessor Macros

Preprocessor macros (defines) should always have a newline before and after except when the preceding or following line
is also a preprocessor macro. For function-style macros, a surrounding newline is still preferred in this case.

### Classes/Structs

Class and struct declarations should generally follow the typical block rules. Superclass declarations may be placed on
the same line, but if a newline is present in any part of the superclass list, there should be a newline immediately
before the colon delimiter (`:`), and a typical continuation indent should be used.

Class/struct definitions should be preceded and followed by one newline in all cases, except when at the beginning or
end of an enclosing block in which case no newline is needed between the class/struct definition and the opening or
closing brace of said block. Class/struct forward declarations do not need to be surrounded by newlines.

### Visibility Specifiers

Visibility specifiers in class/struct definitions should have one preceding newline unless they are on the first line
inside the definition block. They should be indented with two spaces such that their indentation sits between the
enclosing class and any subsequent member definitions.

### Functions

Function declarations should have one space between the return type and name, no whitespace around the opening 
parenthesis, and one space after the closing parenthesis (as described in the block style rules). Each parameter should
have one space between the parameter type and name. Parameters should be separated by a comma with no preceding
whitespace and followed one space.

Member function implementations should have no whitespace around their double-colon delimeter.

Empty functions should place the closing brace on the first line following the declaration with the same indentation
level.

Function declarations and definitions should be preceded and followed by one newline in all cases except when a
class member function declaration immediately follows a visibility specifier, or when they are at the beginning or end
of an enclosing block in which case no newline is needed between the class/struct definition and the opening or closing
closing brace of said block. These newline rules also apply to function forward declarations.

### Constructors

Constructors should generally follow the same whitespace rules as functions, with the addition that member initializers
before the function body should be indented by one level (the same as the function body) and should have no whitespace
around their parentheses.

### If/While/For/Foreach/Switch

`if`, `while`, `for`, and `switch` statements should have one space between the respective keyword and the opening
parenthesis, no space after the opening parenthesis, and no space before the closing parenthesis.

In a `for` statement declaration, each semicolon delimiter (`;`) should have no preceding whitespace and should be
followed by one space.

In a `foreach` statement declaration, the colon operator (`:`) should have one space both before and after.

### Else-if blocks

Else-if block declarations shoud be placed on the same line as the closing brace of the previous block and otherwise
should follow the typical block style.

### Switch blocks

`switch` statements should follow the typical block rules. The corresponding `case` statements should have one space
between the `case` keyword and the value, no space before the colon, and one space between the colon and the opening
brace (if applicable) with the brace being placed on the same line as the `case` statement.

### Ternary statements

Ternary statements should have one space both before and after the question mark operator (`?`) and the colon operator
(`:`). If any part of the ternary statement contains a newline, then the condition, positive expression, and negative
expression should each be on a separate line (and may still be split across multiple lines themselves with
the typical continuation indent). If the ternary statement is split across multiple lines, both operators (`?` and `:`)
should be placed at the start of a line and indented by one level, followed by the corresponding expression on the same
line.

### Initializer lists

Initializer lists surrounded by braces should have one space after the opening brace and one space before the closing
brace. Comma delimiters within the braces should have no preceding whitespace and should be followed by one space.

If there is a newline immediately after the opening brace or immediately before the closing brace, each item in the
initializer list should be placed on its own line and indented by one level, similar to a block. If there is a newline
anywhere else between the braces, a simple continuation indent may be used instead and items need not be separated onto
different lines.

### Lambda Expressions

Lambda expressions should have a space between the capture list and the parameter list (between the closing bracket and
opening parenthesis). If a return type is specified, the arrow operator (`->`) should have one space both before and
after. There should be one space both before and after the opening brace of the lambda body and one space before the
closing brace of the lambda body. If the lambda body consists of only one statement, it may be on the same line as the
rest of the lambda expression provided the entire expression fits on the same line.

### Template Declarations

Template declarations should have a space between the `template` keyword and the opening angle bracket, no whitespace
after the opening angle bracket, and no whitespace before the closing angle bracket. The enclosed parameter types should
follow the general rules for continuation when multiline. Prefer placing newlines between template parameters when
possible rather than within the parameter types or default expressions.

### Pointers/References

Pointers and references share the same whitespace rules. When specifying a pointer type, there should be one space
between the underlying type name and the relevant operator, (`*`, `&`, or `&&`). There should be no whitespace following
the operator.

### Operators

All operators should have one space both before and after, except for the following:

- Mathematical negation (`-`)
- Logical negation (`!`)
- Bitwise negation (`~`)
- Member access (`.`)
- Pointer member access (`->`)
- Reference (`&`)
- Dereference (`*`)

If two expressions separated by an operator are placed on different lines, the operator should be placed after the
newline.

## Pimpl Classes

Argus uses the pimpl pattern extensively. Pimpl stands for "private implementation" and is a pattern where classes
contain a pointer to an opaque pimpl class that contains the actual fields. This allows the class members to be modified
as needed without breaking ABI compatibility.

Classes using this pattern should have a corresponding struct named `pimpl_<ClassName>`. In most cases the "real" class
should contain only a single member field named `pimpl` of type `pimpl_<ClassName> *`. If the class is inherited by
subclasses which need their own specific member fields, then a corresponding hierarchy should be created for the
respective pimpl classes. In this scenario, the superclass should not define a pimpl member of its own and should
instead declare a virtual function called `get_pimpl` which the subclasses must implement. The return type of this
function should be the pimpl class corresponding to the superclass.

As a concrete example: Suppose classes `Foo` and `Bar` extend from class `Xyzzy`, and they both need member fields
specific to their own type. The correpsonding pimpl classes `pimpl_Foo`, `pimpl_Bar`, and `pimpl_Xyzzy` should be
defined with the former two extending from the latter and with `Foo`-specific fields going in `pimpl_Foo`,
`Bar`-specific fields going in `pimpl_Bar`, and common fields going in `pimpl_Xyzzy`. `pimpl_Xyzzy` should declare a
virtual function `get_pimpl` which returns type `pimpl_Xyzzy *`. `pimpl_Foo` and `pimpl_Bar` should contain their own
`pimpl` fields respectively of types `pimpl_Foo` and `pimpl_Bar` and should return these in their own implementations
of `get_pimpl`.

All this said, given that inheritance should generally be avoided, this only actually occurs in a couple of instances
in Argus, so these rules aren't relevant outside of those cases.

## Naming Conventions

The following table should be used for naming conventions of different entity types.

| Entity type            | Case             | Prefix   |
|:-----------------------|:-----------------|:---------|
| Class/Struct           | PascalCase       |          |
| Template struct        | snake_case       |          |
| Pimpl struct           | PascalCase       | `pimpl_` |
| Enum                   | PascalCase       |          |
| Enum value             | PascalCase       |          |
| Union                  | PascalCase       |          |
| Typedef                | PascalCase       |          |
| Namespace              | snake_case       |          |
| Global function        | snake_case       |          |
| Static global function | snake_case       | `_`      |
| Member function        | snake_case       |          |
| Member field           | snake_case       | `m_`     |
| Pimpl member field     | snake_case       |          |
| C struct member field  | snake_case       |          |
| Global variable        | snake_case       | `g_`     |
| Local variable         | snake_case       |          |
| Parameter              | snake_case       |          |
| Global constant        | UPPER_SNAKE_CASE |          |
| Macro                  | UPPER_SNAKE_CASE |          |

The first notable exception here applies to template structs; that is, structs which are specifically used for template
metaprogramming. An example of this is the `function_traits` struct and its specializations, defined in
[`lowlevel/extra_type_traits.hpp`](engine/libs/lowlevel/include/argus/lowlevel/extra_type_traits.hpp).

The second exception is carved out for pimpl structs. Pimpl struct names have the prefix `pimpl_` and  otherwise use
PascalCase, and pimpl struct member names do not have the `m_` prefix. The reasoning behind the exception for member
names is that pimpl structs should not define any member functions (apart from constructors/destructors), and as a
result are only ever access through the `pimpl` field of the enclosing class and thus the prefix is always redundant.

The final exception applies to structs which do not declare member functions. In this case, access to member fields
will always be external and there is no point in differentiating between member fields and other variables. Note that
this always applies to C structs (structs declared in an `extern "C"` block) since C does not support member functions.

### Namespaces

All definitions in Argus should be directly inside the `argus` namespace. The `input` module currently defines a nested 
namespace `input`, but this is the only instance like this and it will eventually be removed.

This applies to `extern "C"` functions as well for consistency, even though they won't actually be contained by the
namespace once compiled.

Nested namespaces should use the simplified syntax introduced in C++17 (`namespace foo::bar {}` as opposed to
`namespace foo { namespace bar {} }`).

## Integer types

Use the integer types from `stdint.h` in all cases. Never use the `int`, `short`, or `long` keywords unless some
external constraint applies. These built-in types are not guaranteed to be of a specific size and thus are (at least on
paper) unpredictable to work with. Apart from this, the explicit sizes in the `stdint.h` type names are clearer than the
built-in keywords.

## Exceptions

Avoid using exceptions. Eventually the plan is to compile with `-fno-exceptions` so that exceptions are completely
disabled.

## Code Hygiene

### Pointers

Do not use pointers if at all avoidable. References should be used in almost all cases. The exceptions to this are when
interfacing with a C ABI which uses pointers (like C strings), and in collections of dynamically allocated objects
(since references cannot be used as template parameters).

### References

Constructor parameters should generally be passed by value, assuming that constructed object is to take ownership of the
object being passed.

Always pass non-primitive function parameters by reference, more specifically a const reference unless the function is
expected to modify the parameter. Never pass primitive parameters by reference unless there is a compelling reason
(generally speaking though, out variables in C++ code should always be avoided).

Always return references from functions which do not return primitive types, unless the function is specifically
intended to transfer ownership of the returned object to the caller.

### Parameter Immutability

Never modify a function parameter within the function unless it is a reference or pointer parameter which the function
is explicitly expected to modify. If a parameter needs to be changed e.g. in each iteration of a loop, create a local
variable instead and initialize it to the parameter's value.

### KISS

**KISS** stands for **K**eep **I**t **S**imple, **S**tupid. Always prefer simpler solutions over more complex ones and
readable code over esoteric code unless there is a compelling reason not to (e.g. a significant performance difference).
Always be as explicit as possible (within reason) when building an expression with multiple dependencies. For instance
(courtesy of [grugbrain.dev](https://grugbrain.dev)), an expression like this:

```java
if (contact && !contact.isActive() && (contact.inGroup(FAMILY) || contact.inGroup(FRIENDS))) {
    // ...
}
```

is much more readable when expressed like this:

```java
if (contact) {
    var contactIsInactive = !contact.isActive();
    var contactIsFamilyOrFriends = contact.inGroup(FAMILY) || contact.inGroup(FRIENDS);
    if (contactIsInactive && contactIsFamilyOrFriends) {
        // ...
    }
}
```

The same concept should be used when applying the DRY (**D**on't **R**epeat **Y**ourself) principle. DRY should
generally be followed, but not when the complexity it introduces is disproportionate to how much deduplicating it's
doing. It's better to repeat a few lines of code once or twice than to introduce an unnecessarily complex pattern or to
tangle the call stack into spaghetti.

### Const-correctness

Always use the `const` attribute for reference/pointer parameters to functions if the function does not and will never
have a reason to modify the parameter. Never use the `const` attribute for parameters which are not a reference or a
pointer, both in function declarations and definitions.

Always use the `const` attribute for member functions which do not and will never have a reason to modify its member
fields. For classes with a pimpl member, members of the pimpl class should be considered to be part of the base class.
In other words, even though the compiler technically allows a `const` attribute on a function that only modifies pimpl
fields, the attribute should _never_ be used in this case. Adding it to such a function is misleading and gives the
false impression that the function will not affect the object's state, abstractly speaking. 

Do not use the `mutable` keyword if at all avoidable. There are vanishingly few cases where this is justifiable -- as of
writing the only use in Argus's codebase is in the `dirty` (dirty flag) member of the `Dirtiable` class. 

### Rule of 3/5/0

Class definitions should always follow
[the Rule of 3, 5, or 0](https://en.cppreference.com/w/cpp/language/rule_of_three). That is, if a class specifies a
user-defined copy constructor, copy-assignment operator, or destructor, it must provide definitions of all three.
Additionally, if a class specifies a user-defined move constructor or move-assignment operator, it must provide
definitions of both in addition to the other three definitions. Absolutely no exceptions to this.

### Annotations

#### `[[nodiscard]]`

Always use the `[[nodiscard]]` function annotation where appropriate. 

## Headers

### Header Guards

Use `#pragma once` as a header guard - `#ifndef CLASS_NAME_H`-style guards should not be used.

### Include What You Use

Source files should include everything they need to compile on their own. In other words, every external definition used
within a given file should have a corresponding `#include` for the header which provides them. This prevents odd compile
errors that can arise when a file only pulls in a dependency transitively, and at some point the transitive chain breaks
so that the dependency is no longer pulled in. Additionally, it makes it easier to mentally parse out the dependency
graph of project or any component thereof.
