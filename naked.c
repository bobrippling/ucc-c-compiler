__attribute__((naked))
int f(int a, int b)
{
	//int x; - non-asm statement not allowed
	//__asm__("hi" : : "r"(a), "r"(b)); - parameter references not allowed
	__asm__("hi");
	__asm__("yo");
	//return 3; - non-asm statement not allowed
}

int main()
{
	int x = f(1, 2);
	return x + 3;
}
