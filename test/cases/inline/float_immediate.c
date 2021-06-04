// RUN: %ocheck 1 %s -fno-semantic-interposition

__attribute((always_inline))
inline or(float a)
{
	return a;
}

main()
{
#include "../ocheck-init.c"
	return or(1);
}
