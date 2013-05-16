#define J(a, b, ...) __VA_ARGS__ ## a ## b

J(1, 2, 3)
J(x, y)
