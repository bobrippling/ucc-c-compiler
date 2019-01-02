// RUN: %ocheck 0 %s

reset(int *p)
{
  *p = -5;
}

main()
{
  int x = ({
    int n __attribute((cleanup(reset))) = 0;
		// scope leave / destructors were generated here
    n + 1;
		// they're now generated here
  });

  if(x != 1)
		abort();

	return 0;
}
