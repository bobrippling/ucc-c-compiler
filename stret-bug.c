typedef struct A { long a, b; } A;

A f(A *p) {
	return *p;
	// mov p, %rax
	// load from rax[0] -> %rax
	// load from rax[1] -> %rdx // BUG! rax clobbered above
}

// TODO: compile vis when this is fixed
