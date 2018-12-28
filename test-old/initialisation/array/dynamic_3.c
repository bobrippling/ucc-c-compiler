print_c(char c)
{
	extern write(int, *, int);
	write(1, &c, 1);
}

print_n(int n)
{
	n += '0';
	print_c(n);
}

print_ar(int a[], int n)
{
	while(n --> 0)
		print_n(*a++), print_c(' ');

	print_c('\n');
}

main()
{
	for(int i = 0; i < 10; i++){
		int x[] = { 4, 3, 2 };
		x[1] = i;
		print_ar(x, 3);
	}
}
