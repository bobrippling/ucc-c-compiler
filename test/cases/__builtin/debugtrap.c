// RUN: %ucc -target x86_64-linux -S -o %t %s
// RUN: grep int3 %t

void f()
{
	__builtin_debugtrap();
}
