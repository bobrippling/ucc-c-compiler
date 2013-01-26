// RUN: %ucc -DTYPE='int[]' etc etc...
// RUN: %ucc -DTYPE='int()' etc etc...

main()
{
	return (TYPE)0;
}
