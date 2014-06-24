// RUN: %ocheck 0 %s
// RUN: %archgen %s 'x86,x86_64:/addl/' 'x86,x86_64:!/addb/'

main()
{
	for(unsigned char c = 1; c > 0; c++)
		;

	char c = 3;

	c += (int)1; // this should generate addl, not addb
	// i.e. integer promotion rules (for compound assign)

	if(c != 4)
		abort();

	return 0;
}
