__attribute__((naked))
int f(int a, int b)
{
	//int x; - non-asm statement not allowed
	//__asm__("hi" : : "r"(a), "r"(b)); - parameter references not allowed
	__asm__("hi");
	__asm__("yo");
	//return 3; - non-asm statement not allowed
}

__attribute__((naked))
int f(int x, int y)
{
    __asm__ (
        "lea (%rdi, %rsi, 1), %rax\n"
        "ret\n"
    );
}

__attribute__((naked))
int g(int x, int y)
{
    __asm__ (
        "push %rbp"
        "mov %rsp, %rbp"
        "lea (%rdi, %rsi, 1), %rax\n"
        "pop %rbp"
        "ret\n"
    );
}

int main()
{
	int x = f(1, 2);
	return x + 3;
}
