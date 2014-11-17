// RUN: %ocheck 0 %s

struct A
{
	// rdx:rax
	// f-k:j-i
	int a, b, c;
	float d;
} af()
{ return (struct A){ 1, 2, 3, 4 }; }

struct B
{
	// xmm0:rax
	//  b-a:j-i
	int a, b;
	float c, d;
} bf()
{ return (struct B){ 1, 2, 3, 4 }; }

struct C
{
	// rdx:rax
	// b-j:a-i
	int a;
	float b;
	int c;
	float d;
} cf()
{ return (struct C){ 1, 2, 3, 4 }; }

struct D
{
	// rdx:rax
	// j-b:a-i
	int a;
	float b, c;
	int d;
} df()
{ return (struct D){ 1, 2, 3, 4 }; }

struct E
{
	// rdx:rax
	// b-j:i-a
	float a;
	int b, c;
	float d;
} ef()
{ return (struct E){ 1, 2, 3, 4 }; }

struct F
{
	// rax:xmm0
	// j-i:b-a
	float a, b;
	int c, d;
} ff()
{ return (struct F){ 1, 2, 3, 4 }; }

#define CHECK(st) \
	if(st.a != 1    \
	|| st.b != 2    \
	|| st.c != 3    \
	|| st.d != 4)   \
	{               \
		abort();      \
	}
extern void abort(void);

int main()
{
	struct A a = af();
	CHECK(a);

	struct B b = bf();
	CHECK(b);

	struct C c = cf();
	CHECK(c);

	struct D d = df();
	CHECK(d);

	struct E e = ef();
	CHECK(e);

	struct F f = ff();
	CHECK(f);
}
