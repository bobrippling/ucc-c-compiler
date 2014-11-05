// RUN: %ocheck 10 %s -finline-functions

void abort(void);

__attribute((always_inline))
inline void fill(int n, short vla[n])
{
	for(int i = 0; i < n; i++)
		vla[i] = 3;
}

gs, hs, qs;

g(){ gs++; return 3; }
h(){ hs++; return 4; }
q(){ qs++; return 5; }

static inline int f(int n, int vla[g()][h()])
{
	// 5 + 5 = 10
	short lvla[n + q()];

	vla[1][2] = 7;

	int sz = sizeof lvla;
	if(sz != sizeof(short) * 10)
		abort();

	// 10, lvla
	fill(sz / sizeof *lvla, lvla);

	return lvla[2]; // 3
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

	// 3 + 7
	return r + vla[1][2];
}
