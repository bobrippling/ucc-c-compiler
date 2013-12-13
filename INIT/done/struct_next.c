struct
{
	struct
	{
		int a, b;
	} A;
	int k;
} tim = {
	.A.a = 1,
	2
};

/*
main()
{
	printf("tim = { .A = { .a = %d, .b = %d}, .k = %d\n",
			tim.A.a, tim.A.b, tim.k);
}
*/
