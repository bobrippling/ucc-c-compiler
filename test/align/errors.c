// RUN: %check --only -e %s -Wno-sym-never-written

typedef _Alignas(8) int aligned_int; // CHECK: error: typedefs can't be aligned

_Alignas(1) int i; // // CHECK: error: can't reduce alignment (4 -> 1)

_Alignas(3) char c; // CHECK: error: alignment 3 isn't a power of 2

_Alignas(2) void f() // CHECK: error: alignment specified for function 'f'
{
	_Alignas(8) register eax; // CHECK: error: can't align register int eax
}
