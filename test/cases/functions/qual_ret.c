// RUN: %ucc -fsyntax-only %s
// RUN: %check --only -e %s -DERROR

const void f() // CHECK: warning: function has qualified void return type (const)
{
}

double h(void);
const double h(void); // CHECK: warning: const qualification on return type has no effect

main()
{
	double (*hp)(void) = h;
	double const (*hpc)(void) = hp; // no (relevant) warnings, qualifiers on return types are ignored in the type system
	// CHECK: ^warning: const qualification on return type has no effect
}

#ifdef ERROR
int x = _Generic(&h,
		double (*)(void): 1,
		double const (*)(void): 2 // CHECK: error: duplicate type in _Generic: double (*)(void)
		// CHECK: ^warning: const qualification on return type has no effect
);
#endif
