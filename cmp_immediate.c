gt_42(int a)
{
	// cmp $-42, %edi
	// $-42 is sign extended to the length of the other operand
	// ensure this still behaves correctly
	return a > -42;
}

show(int x)
{
	int printf();
	printf("%d > -42 = %d\n", x, gt_42(x));
}

main()
{
	show(-41);
	show(-42);
	show(-43);
}
