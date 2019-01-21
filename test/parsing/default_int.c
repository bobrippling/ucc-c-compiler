// RUN: %check %s -w -Wimplicit

static x; // CHECK: warning: defaulting type to int
f() // CHECK: warning: defaulting type to int
{
	static x; // CHECK: warning: defaulting type to int
	auto z; // CHECK: warning: defaulting type to int
}
y; // CHECK: warning: defaulting type to int

static int x2; // CHECK: !/warn/
int f2() // CHECK: !/warn/
{
	static int x2; // CHECK: !/warn/
	auto int z2; // CHECK: !/warn/
}
int y2; // CHECK: !/warn/
