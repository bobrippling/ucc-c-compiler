main()
{
	__asm("%0 %1"
			:
			: "c"(3), "c"(5)); // 'c' has to be 3 and 5
}
