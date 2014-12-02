main()
{
	int i = 0;

	__asm goto ("jmp %l1" :: "mr"(i) :: lbl );
	__builtin_unreachable();

	i = 1;

lbl:
	return i;
}
