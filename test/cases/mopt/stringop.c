// RUN:   %ucc -DSZ=16 -mstringop-strategy=libcall -S -o- %s | grep memcpy
// RUN: ! %ucc -DSZ=16 -mstringop-strategy=loop -S -o- %s | grep memcpy
//
// RUN:   %ucc -DSZ=16 -mstringop-strategy=libcall-threshold=16 -S -o- %s | grep memcpy
// RUN: ! %ucc -DSZ=15 -mstringop-strategy=libcall-threshold=16 -S -o- %s | grep memcpy

typedef struct A { char bytes[SZ]; } A;

void g(A *);

void f(A *p)
{
	A a = *p;
	g(&a);
}
