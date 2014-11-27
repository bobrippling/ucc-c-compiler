// RUN: %ucc -c -o %t %s

f(int a, int b)
{
	__asm__("got %0 %1"
			: "+g"(*(int *)(a == b))
			: "g"(a == b));
}
