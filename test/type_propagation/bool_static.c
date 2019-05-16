// RUN: %ocheck 2 %s

_Bool x = 5;

main()
{
	_Bool b = 3;

	return b + (int)x;
}
