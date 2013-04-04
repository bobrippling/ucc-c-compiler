// RUN: %ocheck 0 %s
// RUN: %output_check %s yo

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
