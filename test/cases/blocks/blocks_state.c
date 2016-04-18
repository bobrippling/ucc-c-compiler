// RUN: %ucc -o %t %s
// RUN: %t | %output_check 2 7 5 hi
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));
int the_i;

int (^makeadder(int i))(int)
{
	the_i = i;
	return ^(int a){return a + the_i;};
}

main()
{
	int (^f)() = ^{return 2;};
	int (^*i)() = &f;
	int (^adder)(int);

	printf("%d\n", (*i)());

	adder = makeadder(5);
	printf("%d\n", adder(2)); // 7

	printf("%d\n", makeadder(3)(2));

	^(const char *s){printf("%s\n", s);}("hi");
}
