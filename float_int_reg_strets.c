struct A
{
	// rdx:rax
	// f-k:j-i
	int i, j, k;
	float f;
} af()
#ifdef IMPL
{ return (struct A){ 1, 2, 3, 4 }; }
#else
;
#endif

struct B
{
	// xmm0:rax
	//  b-a:j-i
	int i, j;
	float a, b;
} bf()
#ifdef IMPL
{ return (struct B){ 1, 2, 3, 4 }; }
#else
;
#endif

struct C
{
	// rdx:rax
	// b-j:a-i
	int i;
	float a;
	int j;
	float b;
} cf()
#ifdef IMPL
{ return (struct C){ 1, 2, 3, 4 }; }
#else
;
#endif

struct D
{
	// rdx:rax
	// j-b:a-i
	int i;
	float a, b;
	int j;
} df()
#ifdef IMPL
{ return (struct D){ 1, 2, 3, 4 }; }
#else
;
#endif

struct E
{
	// rdx:rax
	// b-j:i-a
	float a;
	int i, j;
	float b;
} ef()
#ifdef IMPL
{ return (struct E){ 1, 2, 3, 4 }; }
#else
;
#endif

struct F
{
	// rax:xmm0
	// j-i:b-a
	float a, b;
	int i, j;
} ff()
#ifdef IMPL
{ return (struct F){ 1, 2, 3, 4 }; }
#else
;
#endif

#ifndef IMPL
int main()
{
	extern int printf(const char *, ...) __attribute((format(printf, 1, 2)));

	struct A a = af();
	printf("%d %d %d %f\n", a.i, a.j, a.k, a.f);

	struct B b = bf();
	printf("%d %d %f %f\n", b.i, b.j, b.a, b.b);

	struct C c = cf();
	printf("%d %f %d %f\n", c.i, c.a, c.j, c.b);

	struct D d = df();
	printf("%d %f %f %d\n", d.i, d.a, d.b, d.j);

	struct E e = ef();
	printf("%f %d %d %f\n", e.a, e.i, e.j, e.b);

	struct F f = ff();
	printf("%f %f %d %d\n", f.a, f.b, f.i, f.j);
}
#endif
