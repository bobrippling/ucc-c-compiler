// RUN: %check -e %s

#define force_const(a)         \
	__builtin_choose_expr(       \
			__builtin_constant_p(a), \
			a,                       \
			(void)0)

int x = force_const(f()); // CHECK: /error: initialisation from void expression/
