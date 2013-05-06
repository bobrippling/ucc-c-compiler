// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 0 ]
// RUN: %t | sed -n 1p | grep 'hi 5'
// RUN: %t | sed -n 2p | grep '^a$'

main()
{
	int (^f)(int) = ^int (int i) {printf("hi %d\n", i); return 0;};

	printf("%c\n", ^char {
		return 'a';
	}());

	return f(5);
}
