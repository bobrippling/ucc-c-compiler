// RUN: %ucc -fsyntax-only %s

typedef struct type type;
type *const qual_t;

// 1
enum { I = __builtin_types_compatible_p(type *, __typeof(qual_t)) };

// 1
enum { J = _Generic(qual_t, type *: 1, type *const: 2) };

// 3
enum { K = _Generic(qual_t, type *const: 2, default: 3) };

_Static_assert(I == 1, "");
_Static_assert(J == 1, "");
_Static_assert(K == 3, "");
