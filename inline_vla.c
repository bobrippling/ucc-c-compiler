__attribute((always_inline))
void fill(int n, int vla[n])
{
	for(int i = 0; i < n; i++)
		vla[i] = 3;
}

gs, hs, qs;

g(){ gs++; return 3; }
h(){ hs++; return 4; }
q(){ qs++; return 5; }

__attribute((always_inline))
int f(int n, int vla[g()][h()])
{
	short lvla[n + q()];

	vla[1][2] = 7;

	int sz = sizeof lvla;
	if(sz != sizeof(short) * 10)
		abort();

	fill(lvla, sz / sizeof *lvla);

	return lvla[2];
}

int main()
{
	int vla[g()][h()];

	int r = f(5, vla);

	if(hs != 2)
		abort();
	if(gs != 2)
		abort();
	if(qs != 1)
		abort();

	return r + vla[1][2];
}
