// RUN: %ocheck 0 %s

_Bool f(float f)
{
	return f;
}

main()
{
#include "../../ocheck-init.c"
	return f(0);
}
