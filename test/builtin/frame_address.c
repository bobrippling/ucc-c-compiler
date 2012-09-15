main()
{
	int i;
	((int *)__builtin_frame_address(0))[-1] = 3;

	printf("%d\n", i);
}
