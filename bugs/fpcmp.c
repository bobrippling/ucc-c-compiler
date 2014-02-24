static void signed_zero_inf(void)
{
	double x = 0.0, y = -0.0;

	if (x == y)
		printf("Test 1.0 / 0.0 != 1.0 / -0.0 returns %d (should be 1).\n",
				1.0 / x != 1.0 / y);
	else
		printf("0.0 != -0.0; this is wrong!\n");
}

main()
{
	signed_zero_inf();
}
