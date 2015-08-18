int printf(const char *, ...)
	__attribute__((format(printf, 1, 2)));

typedef long double L;
#define FMT "Lf"

void f(L a, L b, L c, L d, L e, L f, L h, L i, L j)
#if defined(IMPL) || defined(ALL)
{
	printf("%" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT " %" FMT "\n",
			a, b, c, d, e, f, h, i, j);
}
#else
;
#endif

#if !defined(IMPL) || defined(ALL)
int main()
{
	f(1, 2, 3, 4, 5, 6, 7, 8, 9);
}
#endif
