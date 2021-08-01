// RUN:   %ucc -target x86_64-linux -S -o- %s          | grep PLT
// RUN: ! %ucc -target x86_64-linux -S -o- %s -fno-plt | grep PLT

int g(void);

int f(void)
{
	return g();
}
