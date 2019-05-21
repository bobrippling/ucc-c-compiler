A C Compiler written in C

[![Build Status]](https://travis-ci.org/bobrippling/ucc-c-compiler)

Dependencies
------------

- C standard library
- `as` (tested with gnu-as and darwin-as [now clang-as], but any should be fine)
- `ld` (any should be fine)
- A C89 compiler to build this compiler with


Features
--------

The compiler implements C89, C99 and C11 (controllable via `-std=c89/c99/c11`).
System libraries are fully supported, including ABI compatibility with constructs such as `va_list`.
There are some major additions, listed below:

- Features added:
	- Microsoft/Plan 9 struct extensions
	- Lambdas/Objective-C style blocks
	- Trailing return types on functions
	- Namespace checking
- Standard support (not exhaustive):
	- C89, C99
	- C11 `_Bool`, `_Noreturn`, `_Alignof`, `_Alignas` `_Generic`, `_Static_assert`
- Feature support:
  - Floating point types
  - Unsigned integers
  - `char`, `short`, `long` and (on 64-bit only) `long long`
  - Wide characters, wide strings
  - `volatile` qualifier
  - `register` storage class
- Common language extensions
	- `__auto_type`
	- GNU extensions (see [below](#gnu-c-supported-extensions) for a complete list)
		- GNU omitted middle operand in `?:`
		- GNU keywords - `asm`, `inline` (in C89), `typeof`, `__extension__`, `__attribute__`, `__label__`, `__alignof__`, `__alignas__`
		- GNU Array-range designated initialiser
	- Computed `goto` / address-of-label (`&&label`)
	- GCC/Clang builtins (not an exhaustive list):
		- `__builtin_unreachable`, `__builtin_trap`
		- `__builtin_types_compatible_p`
		- `__builtin_constant_p`
		- `__builtin_frame_address`
		- `__builtin_expect`
		- `__builtin_choose_expr`
		- `__builtin_add_overflow`, `__builtin_sub_overflow`, `__builtin_mul_overflow`
		- `__builtin_frame_address`, `__builtin_return_address`
- Code-generation Support/Optimisations:
	- Function inlining (`__attribute__((always_inline/noinline))`)
	- Position independent code generation (`-fpic`)
	- Position independent executable generation, permitting ASLR (`-fpie` / `-pie`)
	- Overflow-trapping arithmetic (`-ftrapv`)
	- Undefined behaviour trapping (`-fsanitize=undefined`)
	- Stack protector (`-fstack-protector`, `-fstack-protector-all`)
	- Symbol visibility (`-fvisibility=default/protected/hidden` / `-f[no-]semantic-interposition` / `__attribute__((visibility(...)))`)
	- DWARF Debug Symbols (`-g` / `-gline-tables-only`)


Extensions
----------

### Lambdas:
```c
^(parameters, ...) { body }
^T (parameters, ...) { body }
^T { body }
^ { body }
```

The syntax for lambdas/blocks is similar to that in Objective-C. Closing over external variables isn't implemented yet, nor is the `__block` keyword.

Other forms with explicit return types, omitted parameters, and omitted parameters and return types are allowed.

When the return type is omitted, the return type is inferred from the first return statement in the body, or `void`, if there are none.
The result of the expression is a function block pointer (`T (^)(Args...)`), explicitly convertible to a function pointer.

### Namespace checking:
```c
#pragma ucc namespace expr_
```

This ensures that any declarations after this `pragma` begin with `expr_`, allowing you to enforce a namespace exported by each translation unit (`.c` file).

See [namespace.c](/test/pragma/namespace.c) for an example.


GNU C Supported Extensions
----------

Summarised from the GNU C [Extensions page].

Supported | Extension Name | Description
--------- | -------------- | -----------
✅ Yes | Statement Exprs | Putting statements and declarations inside expressions.
✅ Yes | Local Labels | Labels local to a block.
✅ Yes | Labels as Values | Getting pointers to labels, and computed gotos.
❌ No | Nested Functions | Nested function in GNU C.
✅ Yes | Nonlocal Gotos | Nonlocal gotos.
❌ No | Constructing Calls | Dispatching a call to another function.
✅ Yes | Typeof | typeof: referring to the type of an expression.
✅ Yes | Conditionals | Omitting the middle operand of a `?:` expression.
❌ No | __int128 | 128-bit integers-__int128.
🖥️ 64-bit targets only | Long Long | Double-word integers-long long int.
❌ No | Complex | Data types for complex numbers.
❌ No | Floating Types | Additional Floating Types.
❌ No | Half-Precision | Half-Precision Floating Point.
❌ No | Decimal Float | Decimal Floating Types.
✅ Yes | Hex Floats | Hexadecimal floating-point constants.
❌ No | Fixed-Point | Fixed-Point Types.
❌ No | Named Address Spaces | Named address spaces.
✅ Yes | Zero Length | Zero-length arrays.
✅ Yes | Empty Structures | Structures with no members.
✅ Yes | Variable Length | Arrays whose length is computed at run time.
❌ No | Variadic Macros | Macros with a variable number of arguments. `#define f(a, b...) ...`
✅ Yes | Escaped Newlines | Slightly looser rules for escaped newlines.
❌ No | Subscripting | Any array can be subscripted, even if not an lvalue. (This is intentionally not supported)
✅ Yes | Pointer Arith | Arithmetic on void-pointers and function pointers.
✅ Yes | Variadic Pointer Args | Pointer arguments to variadic functions.
✅ Yes | Pointers to Arrays | Pointers to arrays with qualifiers work as expected.
✅ Yes | Initializers | Non-constant initializers.
✅ Yes | Compound Literals | Compound literals give structures, unions or arrays as values.
✅ Yes | Designated Inits | Labeling elements of initializers.
✅ Yes | Case Ranges | `case 1 ... 9` and such.
❌ No | Cast to Union | Casting to union type from any member of the union.
✅ Yes | Mixed Declarations | Mixing declarations and code.
🔎 Partial | Function Attributes | Declaring that functions have no side effects, or that they can never return.
🔎 Partial | Variable Attributes | Specifying attributes of variables.
🔎 Partial | Type Attributes | Specifying attributes of types.
✅ Yes | Label Attributes | Specifying attributes on labels.
✅ Yes | Enumerator Attributes | Specifying attributes on enumerators.
❌ No | Statement Attributes | Specifying attributes on statements. `__attribute__((fallthrough));`
✅ Yes | Attribute Syntax | Formal syntax for attributes.
✅ Yes | Function Prototypes | Prototype declarations and old-style definitions.
✅ Yes | C++ Comments | C++ comments are recognized.
✅ Yes | Dollar Signs | Dollar sign is allowed in identifiers.
✅ Yes | Character Escapes | `\e` stands for the character ESC.
✅ Yes | Alignment | Determining the alignment of a function, type or variable.
✅ Yes | Inline | Defining inline functions (as fast as macros).
✅ Yes | Volatiles | What constitutes an access to a volatile object.
🛠️ [__asm__ WIP] | Using Assembly Language with C | Instructions and extensions for interfacing C with assembler.
✅ Yes | Alternate Keywords | `__const__`, `__asm__`, etc., for header files.
✅ Yes | Incomplete Enums | `enum foo;`, with details to follow.
✅ Yes | Function Names | Printable strings which are the name of the current function.
✅ Yes | Return Address | Getting the return or frame address of a function.
❌ No | Vector Extensions | Using vector instructions through built-in functions.
✅ Yes | Offsetof | Special syntax for implementing offsetof.
❌ No | __sync Builtins | Legacy built-in functions for atomic memory access.
❌ No | __atomic Builtins | Atomic built-in functions with memory model.
🔎 Partial | Integer Overflow Builtins | Built-in functions to perform arithmetics and arithmetic overflow checking.
❌ No | x86 specific memory model extensions for transactional memory | x86 memory models.
❌ No | Object Size Checking | Built-in functions for limited buffer overflow checking.
🔎 Partial | Other Builtins | Other built-in functions.
❌ No | Target Builtins | Built-in functions specific to particular targets.
❌ No | Target Format Checks | Format checks specific to particular targets.
❌ No | Pragmas | Pragmas accepted by GCC.
✅ Yes | Unnamed Fields | Unnamed struct/union fields within structs/unions.
🛠️ [TLS WIP] | Thread-Local | Per-thread variables.
✅ Yes | Binary constants | Binary constants using the `0b` prefix.

Output/Targets
--------------

`ucc` can generate `x86_64` assembly, and had partial support for `MIPS`, but that's unmaintained at the moment. There are plans to add `arm` too.
The code generator can target Linux-, Cygwin- and Darwin-based toolchains (handling differences in PLT calls, leading underscores, stack alignment, etc)

Constant folding and some small amount of optimisation is done, but nothing heavy (the `feature/ir` branch plans to change this).

The ABI matches GCC and Clang's, or more specifically, the System V x86-64 psABI (modulo bugs, of which there is currently one - see [nested_ret.c](/test/structs/function_passing/nested_ret.c)).

`ucc` can also dump its AST, similarly to clang, with `-emit=dump`.


Building
--------

```sh
make
```

If you plan on building the shim libc, or customising `CFLAGS`:
```sh
./configure [CC=...] [CFLAGS=...] [LDFLAGS=...]
```

Installing
----------

`ucc` doesn't have a make install target yet. When run locally, `ucc` will use its own include files for `stdarg.h`, etc, but otherwise will use system includes and libraries.


Compiling C files
-----------------

POSIX 'cc' standard arguments, plus many additions, see `./ucc --help` for details.


Todo
----

- By-value argument passing for structure/union types (`feature/1st-class-struct-args`)
- `long long` types on 32-bit archs
- `long double` type
- Complex types (`_Complex`)
- Atomic types (`_Atomic`)
- Thread local storage (`_Thread_local`, `__thread`)


Limitations/Known Bugs
----------------------

- The preprocessor will fail to expand the latter of several function macros all on the same line (such as glibc's `tgmath.h`)
- The preprocessor can't handle function macros that cross several lines
- `__asm__` statements are incomplete (see branch `feature/asm`)


Examples
--------

`./ucc -o hello hello.c`
- preprocess, compile, assemble and link hello.c into hello

`./ucc -o- -S test.c`
- output assembly code for test.c

`./ucc -o- -S -emit=dump test.c`
- show the abstract parse tree

`./ucc -c test.c`
- (preprocess) compile and assemble test.c -> test.o

`./ucc -c test.s`
- assemble test.s -> test.o
(preprocessing and compilation are skipped)

`./ucc test.c a.o -o out b.a`
- preprocess + compile test.c, and link with a.o and b.a to form the executable out

`./ucc a.o b.c -E`
- preprocess b.c - a.o is ignored since it's not linked with

[Build Status]: https://travis-ci.org/bobrippling/ucc-c-compiler.svg?branch=master
[Extensions page]: https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html
[__asm__ WIP]: //github.com/bobrippling/ucc-c-compiler/tree/feature/asm
[TLS WIP]: //github.com/bobrippling/ucc-c-compiler/tree/feature/tls
