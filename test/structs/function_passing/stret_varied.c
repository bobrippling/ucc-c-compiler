// RUN: %ocheck 0 %s
struct A { int i, j; void *p; } f()
{
	return (struct A){ 1, 2, 0 };
}

struct B { int i, j, k; void *p; } g()
{
	return (struct B){ 1, 2, 3, 0 };
}

struct C { int i, j; double d; } h()
{
	return (struct C){ 1, 2, 3 };
}

struct D { int i, j; float f, g, h; } i()
{
	return (struct D){ 1, 2, 3 };
}

struct C2 { double d; int i, j; } h2()
{
	return (struct C2){ 1, 2, 3 };
}

main()
{
	struct A f_ = f();
	if(f_.i != 1 || f_.j != 2 || f_.p != 0) abort();
	struct B g_ = g();
	if(g_.i != 1 || g_.j != 2 || g_.k != 3 || g_.p != 0) abort();
	struct C h_ = h();
	if(h_.i != 1 || h_.j != 2 || h_.d != 3) abort();
	struct D i_ = i();
	if(i_.i != 1 || i_.j != 2 || i_.f != 3 || i_.g || i_.h) abort();
	struct C2 h2_ = h2();
	if(h2_.d != 1 || h2_.i != 2 || h2_.j != 3) abort();

	return 0;
}
