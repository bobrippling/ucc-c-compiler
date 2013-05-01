main()
{
	for(unsigned char c = 1; c > 0; c++)
		printf("%d\n", c);

	char c = 3;

	c += (int)1; // FIXME: this should generate addl, not addb
	// i.e. integer promotion rules (for compound assign)
}
