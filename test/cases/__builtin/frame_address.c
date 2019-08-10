// RUN: %ocheck 0 %s

main()
{
	int i;
	((int *)__builtin_frame_address(0))[-1] = 3;

	return i == 3 ? 0 : 1;
}
