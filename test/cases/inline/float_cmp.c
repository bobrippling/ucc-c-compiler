// RUN: %ocheck 0 %s -fno-semantic-interposition

__attribute((always_inline))
inline ge(float a, float b)
{
	// this checks retain handling in out_op
	return a >= b;
}

main()
{
#include "../ocheck-init.c"
	return ge(3, 5);
}
