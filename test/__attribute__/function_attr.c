// RUN: %check %s

__attribute__((noreturn)) void
	d0 (void),
	__attribute__((format(printf, 1, 2))) d1 (const char *, ...),
	d2 (void);

// noreturn applies to all
// format applies to d1

_Noreturn void a() // CHECK: !/warning: control reaches end of/
{
	d0();
}


_Noreturn void b() // CHECK: !/warning: control reaches end of/
{
	d2();
}

_Noreturn void c() // CHECK: !/warning: control reaches end of/
{
	d1("hi %s"); // CHECK: warning: too few arguments for format (%s)
}
