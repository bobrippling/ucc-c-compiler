// RUN: test $(%ucc -S -o- %s | grep -c memset) -eq 2
// RUN: test $(%ucc -S -o- %s | grep -c memcpy) -eq 1
// RUN: test $(%ucc -S -o- %s -ffreestanding | grep -c mem) -eq 0

main()
{
	struct A {
		char buf[1024];
		int x;
	} a = { .x = 1 };

	char buf2[1024] = {
		0,
		[242] = 0
	};

	struct A b = a;
}
