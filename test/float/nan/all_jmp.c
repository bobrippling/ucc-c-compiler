// RUN: %ocheck 0 %s

main()
{
	float const nan = __builtin_nanf("");
	float const abc = 5;

	if(abc == nan) // nan = false
		return 1;

	if(nan == nan)
		return 2;

	if(abc != nan) // nan = true
		;
	else
		return 3;

	if(nan != nan) // nan = true
		;
	else
		return 4;

	// nan = false
	if(abc > nan)
		return 5;
	if(abc >= nan)
		return 6;
	if(abc < nan)
		return 7;
	if(abc <= nan)
		return 8;

	// nan, nan = false
	if(nan > nan)
		return 9;
	if(nan >= nan)
		return 10;
	if(nan < nan)
		return 11;
	if(nan <= nan)
		return 12;

	return 0;
}
