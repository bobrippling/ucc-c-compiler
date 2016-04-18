// RUN: %ucc -o %t %s
// RUN: %ocheck 0 %t
// RUN: %t | %output_check yo
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

p1(){printf("yo\n");}
main()
{
	struct
	{
		const char *n;
		void (*f)(void);
	} f;

	f.n = "hi";
	f.f = p1;

	f.f();

	return 0;
}
