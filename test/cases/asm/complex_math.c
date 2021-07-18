// RUN: %ucc %s -o %t
// RUN: %t; [ $? -eq 2 ]

int add(int x, int y) {
	int a, b;
	do {
		a = x & y;
		b = x ^ y;
		x = a << 1;
		y = b;
	} while (a);
	return b;
}

int divideby3 (int num) {
	int sum = 0;
	while (num > 3) {
		sum = add(num >> 2, sum);
		num = add(num >> 2, num & 3);
	}
	if (num == 3)
		sum = add(sum, 1);
	return sum;
}
main()
{
	//printf("%d\n", divideby3(6));
	return divideby3(6);
}
