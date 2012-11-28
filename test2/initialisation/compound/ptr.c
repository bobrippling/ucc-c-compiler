main()
{
	int (*p)[] = (int[][2]){ 1, 2, { 3, 4, 5 } };
	// should be: { { 1, 2 }, { 3, 4 } } - 5 ignored
}
