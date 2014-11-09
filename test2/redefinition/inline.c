// RUN: %check -e %s

inline int f() // CHECK: note: previous definition
{
	return 3;
}

static int f(); // CHECK: error: static redefinition of non-static "f"

__attribute((noinline))
inline int a() // CHECK: note: previous definition
{
	return 2;
}
static int a(); // CHECK: error: static redefinition of non-static "a"
