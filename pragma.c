#pragma STDC FENV_ACCESS on
_Pragma("STDC FENV_ACCESS on");


#define LISTING(x) PRAGMA(listing on #x)
#define PRAGMA(x) _Pragma(#x)

LISTING(../listing)


_Pragma(L"STDC CX_LIMITED_RANGE off");

#pragma STDC FP_CONTRACT off
