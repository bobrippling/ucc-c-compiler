// RUN: %check --only %s -Wno-cast-qual

struct A;

char *a = (const char *)0; // CHECK: warning: mismatching types, initialisation
                           // CHECK: ^note: 'char *' vs 'char const *'

void f(const struct A *kp)
{
	struct A *p = kp; // CHECK: warning: mismatching types, initialisation
                    // CHECK: ^note: 'struct A *' vs 'struct A const *'
	(void)p;
}

void k(const int *p)
{
	int *kp = p; // CHECK: warning: mismatching types, initialisation
               // CHECK: ^note: 'int *' vs 'int const *'
	(void)kp;
}

void vf(volatile struct A *kp)
{
	struct A *p = kp; // CHECK: warning: mismatching types, initialisation
                    // CHECK: ^note: 'struct A *' vs 'struct A volatile *'
	(void)p;
}

void vk(volatile int *vp)
{
	int *p = vp; // CHECK: warning: mismatching types, initialisation
               // CHECK: ^note: 'int *' vs 'int volatile *'
	(void)p;
}

void g(const char **);
void h(char **p)
{
	g(p); // CHECK: warning: mismatching nested types, argument 1 to g
        // CHECK: ^note: 'char const **' vs 'char **'
}

void x(const char ***);
void y(char ***p)
{
	x(p); // CHECK: warning: mismatching nested types, argument 1 to x
        // CHECK: ^note: 'char const ***' vs 'char ***'
}
