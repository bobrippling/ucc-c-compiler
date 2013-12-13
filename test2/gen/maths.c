// RUN: %ucc -fsyntax-only %s
binary(a, b)
{
	p(a + b);
	p(a - b);
	p(a * b);
	p(a / b);
	p(a % b);

	p(a == b);
	p(a != b);
	p(a <= b);
	p(a < b);
	p(a >= b);
	p(a > b);

	p(a | b);
	p(a ^ b);
	p(a & b);

	p(a << b);
	p(a >> b);
}

unary(a)
{
	p(!a);
	p(~a);
	p(+a);
	p(-a);
}
