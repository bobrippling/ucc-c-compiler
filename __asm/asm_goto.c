main()
{
	int i = 0;

	asm goto ("jmp %l1" :: "mr"(i) :: tim );
	__builtin_unreachable();

	i = 1;

tim:
	return i;
}
