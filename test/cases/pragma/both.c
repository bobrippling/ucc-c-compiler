// RUN: %check %s

#pragma STDC FENV_ACCESS ON
_Pragma("STDC FENV_ACCESS ON"); // CHECK: warning: unhandled STDC pragma
// CHECK: ^warning: extra ';' at global scope


#define LISTING(x) PRAGMA(listing on #x)
#define PRAGMA(x) _Pragma(#x)

LISTING(../listing) // CHECK: warning: unknown pragma 'listing on "../listing"'


_Pragma(L"STDC CX_LIMITED_RANGE OFF") // CHECK: warning: unhandled STDC pragma
// CHECK: ^!/warning: extra ';'/

#pragma STDC FP_CONTRACT OFF

int main()
{
}
