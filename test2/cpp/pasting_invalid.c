// RUN: echo TODO; false
#define G(g) g * 5
#define F(x, y) x + #y + y ## x
F(2, G(3))
