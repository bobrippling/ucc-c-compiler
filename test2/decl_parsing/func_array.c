// RUN: %ocheck 20 %s

(*f[4])(int);
int t;

_Static_assert(sizeof(f) == 32, "bad fptr sz");

#define P(n) \
void p ## n(int i){ t += i * n; }

P(1)
P(2)
P(3)
P(4)

main()
{
	f[0] = p1;
	f[1] = p2;
	f[2] = p3;
	f[3] = p4;

	for(int i = 0; i < 4; i++)
		f[i](i);

	return t;
}
