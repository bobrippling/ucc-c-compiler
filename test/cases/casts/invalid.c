// RUN: ! %ucc -DTYPE='int[]' %s
// RUN: ! %ucc -DTYPE='int()' %s

main()
{
	return (TYPE)0;
}
