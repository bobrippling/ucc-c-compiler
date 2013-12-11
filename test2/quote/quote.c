// RUN: %ocheck 0 %s

assert(x)
{
	if(!x)
		abort();
}

main()
{
	assert('"' + '\"' == 68);

	assert('/*' == 12074);

	return 0;
}
