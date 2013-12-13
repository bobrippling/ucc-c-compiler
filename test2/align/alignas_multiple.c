// RUN: %ucc -c %s

_Alignas(8) _Alignas(4) int i;
_Alignas(2) _Alignas(32) short j;

#define CHECK_ALIGN(v, n) \
_Static_assert(_Alignof(v) == n, #v " != " #n)

CHECK_ALIGN(i, 8);
CHECK_ALIGN(j, 32);
