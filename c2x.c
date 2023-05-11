/*
from https://gcc.gnu.org/gcc-11/changes.html

- As in C++, function definitions no longer need to give names for unused function parameters.
- The [[nodiscard]] standard attribute is now supported.
- Labels may appear before declarations and at the end of a compound statement.
*/

/*
from https://gcc.gnu.org/gcc-13/changes.html

- N3042, Introduce the nullptr constant
- N2963, Enhanced Enumerations (fixed underlying types)
- N2975, Relax requirements for variadic parameter lists
- N3007, Type inference for object definitions (auto)
- N3018, The constexpr specifier for object definitions
- N3038, Introduce storage-class specifiers for compound literals
- typeof (previously supported as an extension) and typeof_unqual
- New keywords alignas, alignof, bool, false, static_assert, thread_local, true
- N2764, The noreturn attribute
- Support for empty initializer braces
- __STDC_VERSION_*_H__ header version macros
- Removal of ATOMIC_VAR_INIT
- unreachable macro in <stddef.h>
- Removal of trigraphs
- Removal of unprototyped functions
- printf and scanf format checking with -Wformat for %wN and %wfN format length modifiers
- N2836, Identifier Syntax using Unicode Standard Annex 31

- -Wenum-int-mismatch warns about mismatches between an enumerated type and an integer type (PR105131)
*/

[[nodiscard]]
int f(int)
{
	l: int x;
	x = 3; // in scope?

	if(0)
		goto m;

	return x;
m:
}

int main()
{
	return f(3);
}

/*
from https://lists.nongnu.org/archive/html/tinycc-devel/2021-06/msg00006.html

+ void f(int) {} (N2480)
+ single-argument _Static_assert (N2265) (already implemented)
+ get rid of k&r definitions (N2432)
  + also: void foo(){} is equivalent to void foo(void){}.  The meaning of
    void foo() (just a prototype) is unchanged + allow labels to precede
    declarations (e.g. lb: int x;) (N2508) + 'true' and 'false' have type
    bool (N2393)
  - use of booleans in arithmetic expressions is 'deprecated'; consider
    emiting warnings?
+ support binary literals (N2549, already supported as a gnu ext.
  Actually as a 'tcc ext', but it's a gnu ext)
- u8'x' -> (unsigned char)'x' (N2418)
  - u8"", U"" et al (from c11) are already not supported, need to add them
- mixed-type literal concatenation is now illegal (u"" U"" is an error;
  "" u"" is fine though.  See above) (N2594)
- Change rules about conversions of pointers to qualified arrays (N2607)
- [[attributes]] (N2335)
*/

// see also https://clang.llvm.org/c_status.html
