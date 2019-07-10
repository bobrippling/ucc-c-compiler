// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'A: 000000ff, B: ffffffff'
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	unsigned char a = 0xff;
	char b = 0xff;

	printf("A: %08x, B: %08x\n", a, b);

	return 0;
}
