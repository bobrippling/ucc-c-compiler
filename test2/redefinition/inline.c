// RUN: %check -e %s

inline int f() // CHECK: note: previous definition
{
	return 3;
}

static int f(); // CHECK: error: static redefinition of non-static "f"
