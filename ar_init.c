print_ar(int a[], int n)
{
	while(n --> 0)
		printf("%d ", *a++);
	putchar(10);
}

main()
{
	for(int i = 0; i < 10; i++){
		int x[] = { 4, 3, 2 };
		x[1] = 5; // 4 5 2
		print_ar(x, 3);
	}
}
