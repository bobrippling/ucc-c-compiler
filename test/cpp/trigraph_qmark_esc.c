// RUN: %ucc -trigraphs %s -o %t
// RUN %t | grep -F 'what??!'
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	printf("what?\?!\n");
}
