// RUN: %ocheck trap %s -ftrapv -DCALL=trapv
// RUN: %ocheck 0 %s -ftrapv -DCALL=notrapv

#define INT_MAX __INT_MAX__

trapv(int i)
{
	return i + 1;
}

notrapv(unsigned i)
{
	return i + 1;
}

main()
{
	int max = INT_MAX;
	CALL(max);
	return 0;
}
