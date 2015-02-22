// RUN: %ocheck 0 %s

float f = 0x.1p1;

main()
{
	if(f != 0.125f)
		abort();

	return 0;
}
