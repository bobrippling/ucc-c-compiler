// RUN: %ucc -o %t %s
// RUN: %t | %stdoutcheck %s
// RUN: %ucc -o %t %s -fstack-protector-all
// RUN: %t | %stdoutcheck %s

// STDOUT: 1793

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

f(a, b, c, d, e, f, g, h)
{
	return
		  (a << 0)
		+ (b << 1)
		+ (c << 2)
		+ (d << 3)
		+ (e << 4)
		+ (f << 5)
		+ (g << 6)
		+ (h << 7)
		;
}

main()
{
	printf("%d\n", f(1, 2, 3, 4, 5, 6, 7, 8));
}
