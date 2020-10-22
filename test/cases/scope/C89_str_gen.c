// RUN: %ucc -o %t %s
// just need to ensure it links - single string symbol
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

f(){}

main()
{
	printf("yo\n");
	if(f())
		;
}
