// TEST: target linux
// ^ need call-via-plt
//
// RUN:   %ucc -S -o- %s          | grep PLT
// RUN: ! %ucc -S -o- %s -fno-plt | grep PLT

int g(void);

int f(void)
{
	return g();
}
