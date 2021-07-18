#ifndef SANITIZE_OPTS_H
#define SANITIZE_OPTS_H

#define SANITIZE_OPTS \
	X(SAN_SHIFT_EXPONENT,            1 << 0,  NO_SANITIZE_UNDEFINED, "shift-exponent",            "a << b, 0 <= b < type-size") \
	X(SAN_VLA_BOUND,                 1 << 1,  NO_SANITIZE_UNDEFINED, "vla-bound",                 "ensure for T vla[N]; that N > 0") \
	X(SAN_SIGNED_INTEGER_OVERFLOW,   1 << 2,  NO_SANITIZE_UNDEFINED, "signed-integer-overflow",   "ensure for +a, a+b that no overflow occurs") \
	X(SAN_BOUNDS,                    1 << 3,  NO_SANITIZE_UNDEFINED, "bounds",                    "ensure array indexes are in bounds") \
	X(SAN_NONNULL_ATTRIBUTE,         1 << 4,  NO_SANITIZE_UNDEFINED, "nonnull-attribute",         "catch null in a __attribute__((nonnull)) argument") \
	X(SAN_INTEGER_DIVIDE_BY_ZERO,    1 << 5,  NO_SANITIZE_UNDEFINED, "integer-divide-by-zero",    "check for a/0, INT_MIN/-1") \
	X(SAN_UNREACHABLE,               1 << 6,  NO_SANITIZE_UNDEFINED, "unreachable",               "__builtin_unreachable becomes a trap") \
	X(SAN_NULL,                      1 << 7,  NO_SANITIZE_UNDEFINED, "null",                      "ensure for *p, p->f(), p != NULL") \
	X(SAN_ALIGNMENT,                 1 << 8,  NO_SANITIZE_UNDEFINED, "alignment",                 "ensure pointers are aligned for the lvalue type") \
	X(SAN_FLOAT_DIVIDE_BY_ZERO,      1 << 9,  NO_SANITIZE_UNDEFINED, "float-divide-by-zero",      "catch floating pointer division-by-zero (not enabled by -fsanitize=undefined)") \
	X(SAN_FLOAT_CAST_OVERFLOW,       1 << 10, NO_SANITIZE_UNDEFINED, "float-cast-overflow",       "catch overflow on floating pointer casts (not enabled by -fsanitize=undefined)") \
	X(SAN_RETURNS_NONNULL_ATTRIBUTE, 1 << 11, NO_SANITIZE_UNDEFINED, "returns-nonnull-attribute", "catch null returned from __attribute__((returns_nonnull))") \
	X(SAN_BOOL,                      1 << 12, NO_SANITIZE_UNDEFINED, "bool",                      "catch storing a value other than 0 or 1 in a _Bool") \
	X(SAN_POINTER_OVERFLOW,          1 << 13, NO_SANITIZE_UNDEFINED, "pointer-overflow",          "catch overflow of pointer arithmetic")

#endif
