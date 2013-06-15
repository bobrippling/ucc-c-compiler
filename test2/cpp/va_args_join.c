// RUN: %ucc -E %s -P | %output_check -w '312' 'xy' 'c, d, eab'
#define J(a, b, ...) __VA_ARGS__ ## a ## b

J(1, 2, 3)
J(x, y)
J(a, b, c, d, e)
