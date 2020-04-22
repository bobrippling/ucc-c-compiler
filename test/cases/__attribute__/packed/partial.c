// RUN: %check --only %s

#define packed __attribute((packed))

struct A
{
	char c;
	packed int i;
};


#define offsetof(S, m) (unsigned long)&((S *)0)->m

#define assert_offset(s, m, n)        \
	_Static_assert(offsetof(s, m) == n, \
			"offsetof(" #s ", " #m ") != " #n)

assert_offset(struct A, c, 0);
assert_offset(struct A, i, 1); // CHECK: warning: taking the address


// warning checks:

struct B {
	packed int x; // CHECK: note: declared here
};

struct packed C { // CHECK: note: declared here
	int x;
};

void f(struct B *p){
	(void)&p->x; // CHECK: warning: taking the address of a packed member
}

void g(struct C *p){
	(void)&(*p).x; // CHECK: warning: taking the address of a packed member
}
