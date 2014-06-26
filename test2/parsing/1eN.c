// RUN: %ocheck 0 %s

x = 56e5;

float y = 1.2e3;

main()
{
	if(x != 5600000)
		abort();
	if(y != 1200)
		abort();
	return 0;
}
