// RUN: %ocheck 0 %s

typedef struct A
{
	long a, b, c;
} A;

static const A s1 = { 1, 2, 3 };
static const A s2 = { 5, 6, 7 };
static A s3;

void g()
{
	A *p = &s3;
	int c = 3;

	*p = *(c ? &s1 : &s2);
	// bug here - the value representing *p would be spilt, and its type changed,
	// meaning the implicit memcpy() from RHS -> LHS would use the wrong type and
	// not copy enough of the struct
	//
	// (previously we'd crash here)
}

/*
static const int i1, i2;
static int i3;

void g_without_lvaluestruct_logic()
{
	typedef int A;
	A *p = &i3;
	int c = 3;

	__builtin_memcpy(p, (c ? &i1 : &i2), sizeof(*p));
}
*/

int main()
{
	int memcmp(const void *, const void *, unsigned long);
	g();
	_Noreturn void abort();
	if(memcmp(&s3, &(A){ 1, 2, 3 }, sizeof(A)))
		abort();
}
