// RUN: %ucc -fsyntax-only %s

main()
{
	((void (*__attribute((format(printf, 1, 2))))())0)();
}
