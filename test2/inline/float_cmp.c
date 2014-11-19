// RUN: %ocheck 0 %s

__attribute((always_inline))
inline ge(float a, float b)
{
	// this checks retain handling in out_op
	return a >= b;
}

main()
{
	return ge(3, 5);
}
