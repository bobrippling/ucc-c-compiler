// RUN: %ucc -E %s -P | %stdoutcheck %s
#define J(a, b, ...) __VA_ARGS__ ## a ## b

J(1, 2, 3)
J(x, y)
J(a, b, c, d, e)

// STDOUT: 312
// STDOUT-NEXT: xy
// STDOUT-NEXT: c, d, eab
