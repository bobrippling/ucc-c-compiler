int main()
{
	int i;
	__asm(
			"movl %1, %0"
			: "=g"(i)
			: "i"(3)
			: "memory"/*, "rax"*/);

	return i;
}
