// RUN: %check %s

struct
	__attribute((designated_init)) // CHECK: note: attribute here
  A
{
	int i;
	int *p;
};


struct A a = { 1, &(int){ 3 } }; // CHECK: warning: positional initialisation of struct

struct A ok = { .p = &ok.i, .i = 3 }; // CHECK: !/warn/

struct A b = { .i = 1, (void *)0 }; // CHECK: warning: positional initialisation of struct

main()
{
	struct A b; // CHECK: !/warn/

	b.i = 3;
	b.p = &b.i;

	struct A c = b; // CHECK: !/warn/
}
