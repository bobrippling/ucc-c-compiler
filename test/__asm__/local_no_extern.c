// RUN: %ucc -fsyntax-only %s

int main()
{
	int f() __asm__("hi");
	f();
}
