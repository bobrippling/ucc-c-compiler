int printf(const char *, ...)
	__attribute__((format(printf, 1, 2)));

typedef long double L;
#define FMT "Lf"

void f(
		L a, L b, L c, L d, L e, L f, L h, L i, L j,
		L a2, L b2, L c2, L d2, L e2, L f2, L h2, L i2, L j2)
#if defined(IMPL) || defined(ALL)
{
	printf(
			"%" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT
			"%" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT
			"\n",
			a, b, c, d, e, f, h, i, j,
			a2, b2, c2, d2, e2, f2, h2, i2, j2);
}
#else
;
#endif

#if !defined(IMPL) || defined(ALL)
int main()
{
	f(1, 2, 3, 4, 5, 6, 7, 8, 9,
		11, 12, 13, 14, 15, 16, 17, 18, 19);
}
#endif
