// RUN: %check -e %s

typedef struct A {
	unsigned long x;
} A;

A q(void);

void h();

void f() {
	A a;

	if(a) // CHECK: error: if-condition requires scalar type (not 'struct A')
		h();

	if(__auto_type x = q()) // CHECK: error: if-condition requires scalar type (not 'struct A')
		h();

	if(A x = q(); x) // CHECK: error: if-condition requires scalar type (not 'struct A')
		h();

	if(A x = q()) // CHECK: error: if-condition requires scalar type (not 'struct A')
		h();
}
