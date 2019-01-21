// RUN: %ucc -E %s -P | %output_check -w '1 + 2 - 3' '312' '3 + 2 + 1' 'data32' 'dataWIDTH'
#define F(a, b, c) a + b - c
#define A F
A(1,2,3)

#define J(a, b, ...) __VA_ARGS__ ## a ## b
J(1, 2, 3)

#define A(x) x + 1
#define B(y) y + 2
A(B(3))

#define WIDTH 32
#define DIRECT(n) data##n
#define INDIRECT(n) DIRECT(n)
INDIRECT(WIDTH)
DIRECT(WIDTH)
