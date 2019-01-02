// RUN: %check %s

int f() __attribute((noreturn)), g();

_Noreturn void a() // CHECK: !/warning: control reaches/
{
	f();
}

_Noreturn void b() // CHECK: warning: function "b" marked no-return implicitly returns
{
	g();
}
