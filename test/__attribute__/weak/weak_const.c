// RUN: %check --only -e %s -Wno-incompatible-pointer-types '-DERROR(...)=__VA_ARGS__'
// RUN: %ucc -fsyntax-only %s '-DERROR(...)=' -Wno-incompatible-pointer-types -Wno-arith-funcptr

enum { false };

__attribute((weak)) void w();
void f();

ERROR(int shortcircuit_weak_1 = w && 1;) // CHECK: error: global scalar initialiser not constant
ERROR(int shortcircuit_weak_2 = w && 0;) // CHECK: error: global scalar initialiser not constant
ERROR(int shortcircuit_weak_3 = 1 && w;) // CHECK: error: global scalar initialiser not constant
_Static_assert((0 && w) == false, "");

ERROR(int shortcircuit_weak_5 = w || 1;) // CHECK: error: global scalar initialiser not constant
ERROR(int shortcircuit_weak_6 = w || 0;) // CHECK: error: global scalar initialiser not constant
_Static_assert(1 || w, "");
ERROR(int shortcircuit_weak_8 = 0 || w;) // CHECK: error: global scalar initialiser not constant

_Static_assert(f && 1, "");
_Static_assert((f && 0) == false, "");
_Static_assert(1 && f, "");
_Static_assert((0 && f) == false, "");

_Static_assert(f || 1, "");
_Static_assert(f || 0, "");
_Static_assert(1 || f, "");
_Static_assert(0 || f, "");

// ---------

void (*arith_fptr_1)() = f + 2; // CHECK: warning: arithmetic on function pointer 'void (*)()'
void (*arith_fptr_2)() = w + 2; // CHECK: warning: arithmetic on function pointer 'void (*)()'

_Static_assert(!&f == false, "");
ERROR(int arith_2 = !&w;) // CHECK: error: global scalar initialiser not constant
_Static_assert(!f == false, "");
ERROR(int arith_4 = !w;) // CHECK: error: global scalar initialiser not constant
_Static_assert((0 == f) == false, "");
ERROR(int arith_6 = 0 == w;) // CHECK: error: global scalar initialiser not constant
_Static_assert(0 != f, "");
ERROR(int arith_8 = 0 != w;) // CHECK: error: global scalar initialiser not constant
ERROR(int arith_9 = f == w;) // CHECK: error: global scalar initialiser not constant
ERROR(int arith_10 = f != w;) // CHECK: error: global scalar initialiser not constant
_Static_assert((f - f) == false, ""); // CHECK: warning: static_assert expression isn't an integer constant expression
ERROR(int arith_12 = w - w;) // CHECK: error: global scalar initialiser not constant
ERROR(int arith_13 = f - w;) // CHECK: error: global scalar initialiser not constant
ERROR(int arith_14 = w - f;) // CHECK: error: global scalar initialiser not constant
