_Alignas(8) _Alignas(4) int i;
_Alignas(2) _Alignas(32) short j;

_Static_assert(alignof(i) == 8, "i != 8");
_Static_assert(alignof(j) == 8, "j != 32");
