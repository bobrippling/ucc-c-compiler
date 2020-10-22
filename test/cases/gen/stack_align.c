// RUN: %ocheck 0 %s
// RUN: %ocheck 0 %s -fstack-protector-all
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

#define NULL (void *)0

p(char *p
#ifdef WITH_ALIGN
		, char *align
#endif
		)
{
	printf("hello %s %s\n", p,
#ifdef WITH_ALIGN
			align
#else
			"n/a"
#endif
			);
}

main()
{
	p("abc"
#ifdef WITH_ALIGN
			, "def"
#endif
			);

	return 0;
}
