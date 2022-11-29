// RUN: %ucc -S -o- %s | grep 'call.*hi'

int main()
{
	int f() __asm__("hi");
	f();
}
