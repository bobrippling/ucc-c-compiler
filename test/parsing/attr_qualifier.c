// RUN: %check --only %s

main()
{
	((void (*__attribute((format(printf, 1, 2))))())0)();

	typedef void (*__attribute((format(printf, 1, 2))) pfn)();
	typedef void (__attribute((format(printf, 1, 2))) fn)();

	((pfn)0)();
	((fn *)0)();

	typedef void (*__attribute((format(printf, 1, 2))) pfn_arg)(const char *, ...);
	typedef void (__attribute((format(printf, 1, 2))) fn)_arg(const char *, ...);

	((pfn)0)("hi %d", 3);
	((fn *)0)("hello %.*s", 2, "x");
	((pfn)0)("hello %*.*s", 1, 2, "x");
}
