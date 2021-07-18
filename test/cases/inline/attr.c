// RUN: %check --only %s -fshow-inlined -fno-semantic-interposition -fno-inline-functions -O1 -Dflatten=
// RUN: %check --only %s -fshow-inlined -fno-semantic-interposition -fno-inline-functions -O1
// RUN: %check --only %s -fshow-inlined -fno-semantic-interposition -fno-inline-functions
// RUN: %check --only %s -fshow-inlined -fno-semantic-interposition

inline int flat_f(void) { return 1; }
inline int flat_g(void) { return 2 + flat_f(); } // CHECK: note: function inlined

__attribute__((flatten))
int flat(void)
{
	return flat_g() * 2; // CHECK: note: function inlined
}

// ----------

__attribute((always_inline))
void f()
{
}

__attribute((noinline))
int g()
{
	f(); // CHECK: note: function inlined
	return 5;
}

int main()
{
	return g(); // not inlined
}
