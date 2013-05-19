//#define F(a, b, c) a + b - c
//#define A F
//A()

//#define J(a, b, ...) __VA_ARGS__ ## a ## b
//J(1, 2, 3)

//#define G(g) g * 5
//#define F(x, y) x + G(y) //x + #y + y ## x
//F(2, G(3))

//#define A(x) x + 1
//#define B(y) y + 2
//A(B(3))

#define WIDTH 32
#define DIRECT(n) data##n
#define INDIRECT(n) DIRECT(n)
INDIRECT(WIDTH)
DIRECT(WIDTH)
