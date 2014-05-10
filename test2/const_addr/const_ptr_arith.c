// RUN: %ocheck 0 %s

// test cmp, addition and (int & ptr) subtraction

// equal, 1
cmp = &((int *)0)[5] == 5 * sizeof(int);

// 12 + 2 * sizeof(int) = 20
add = &((int *)0)[3] + 2;

// 16 sub 2 * sizeof(int) = 8
dif = &((int *)0)[4] - 2;

// 40 - 8 = 32, div sizeof(int) = 8
sub = &((int *)0)[10] - (int *)8;

main()
{
	if(cmp != 1) abort();
	if(add != 20) abort();
	if(dif != 8) abort();
	if(sub != 8) abort();

	return 0;
}
