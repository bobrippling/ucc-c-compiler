// RUN: %ucc -o %t %s
// RUN: %t

main()
{
	return (__builtin_is_signed(unsigned) == 0
			&& __builtin_is_signed(short) == 1) ? 0 : 1;
}
