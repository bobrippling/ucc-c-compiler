#ifndef SANITIZE_OPTS_H
#define SANITIZE_OPTS_H

#define SANITIZE_OPTS \
	X(SAN_SHIFT_EXPONENT,            1 << 0,  "shift-exponent",            "a << b, 0 <= b < type-size") \
	X(SAN_SHIFT_BASE,                1 << 1,  "shift-base",                "a << b, ensure bits aren't shifted off the bottom/top") \
	X(SAN_VLA_BOUND,                 1 << 2,  "vla-bound",                 "ensure for T vla[N]; that N > 0") \
	X(SAN_SIGNED_INTEGER_OVERFLOW,   1 << 3,  "signed-integer-overflow",   "ensure for +a, a+b that no overflow occurs") \
	X(SAN_BOUNDS,                    1 << 4,  "bounds",                    "ensure array indexes are in bounds") \
	X(SAN_NONNULL_ATTRIBUTE,         1 << 5,  "nonnull-attribute",         "catch null in a __attribute__((nonnull)) argument") \
	X(SAN_INTEGER_DIVIDE_BY_ZERO,    1 << 6,  "integer-divide-by-zero",    "check for a/0, INT_MIN/-1") \
	X(SAN_UNREACHABLE,               1 << 7,  "unreachable",               "__builtin_unreachable becomes a trap") \
	X(SAN_NULL,                      1 << 8,  "null",                      "ensure for *p, p->f(), p != NULL") \
	X(SAN_ALIGNMENT,                 1 << 9,  "alignment",                 "ensure pointers are aligned for the lvalue type") \
	X(SAN_FLOAT_DIVIDE_BY_ZERO,      1 << 10, "float-divide-by-zero",      "catch floating pointer division-by-zero (not enabled by -fsanitize=undefined)") \
	X(SAN_FLOAT_CAST_OVERFLOW,       1 << 11, "float-cast-overflow",       "catch overflow on floating pointer casts (not enabled by -fsanitize=undefined)") \
	X(SAN_RETURNS_NONNULL_ATTRIBUTE, 1 << 12, "returns-nonnull-attribute", "catch null returned from __attribute__((returns_nonnull))") \
	X(SAN_BOOL,                      1 << 13, "bool",                      "catch storing a value other than 0 or 1 in a _Bool") \
	X(SAN_ENUM,                      1 << 14, "enum",                      "catch out-of-bounds enum stores") \
	X(SAN_POINTER_OVERFLOW,          1 << 15, "pointer-overflow",          "catch overflow of pointer arithmetic")

#endif
