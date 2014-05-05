// RUN: %check %s

#pragma STDC FENV_ACCESS on
_Pragma("STDC FENV_ACCESS on"); // CHECK: warning: unhandled STDC pragma
// CHECK: ^warning: extra ';' at global scope


#define LISTING(x) PRAGMA(listing on #x)
#define PRAGMA(x) _Pragma(#x)

LISTING(../listing) // CHECK: warning: unknown pragma 'listing on "../listing"'


_Pragma(L"STDC CX_LIMITED_RANGE off") // CHECK: warning: unhandled STDC pragma
// CHECK: ^!/warning: extra ';'/

#pragma STDC FP_CONTRACT off

main()
{
}
