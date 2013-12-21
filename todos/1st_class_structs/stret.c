#if 0
struct A { int i, j; void *p; } f()
{
	return (struct A){ 1, 2, 0 };
}

//rdx <-- i in bits 63-32 and j in bits 31-0
//rax <-- p

struct B { int i, j, k; void *p; } g()
{
	return (struct B){ 1, 2, 3, 0 };
}

struct C { int i, j; double d; } h()
{
	return (struct C){ 1, 2, 3 };
}
#endif

struct D { int i, j; /*float f, g, h, q;*/float f, g, h; } i()
{
	return (struct D){ 1, 2, 3 };
}
