// RUN: %ocheck 0 %s

x = 56e5;

float y = 1.2e3;

z = 7959571e-3;

float q = 1e3f;

main()
{
	if(x != 5600000)
		abort();
	if(y != 1200)
		abort();
	if(z != 7959)
		abort();
	return 0;
}
