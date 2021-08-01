// RUN: %ucc -c %s

#define VAL (1LL << 39)

typedef long long ll;

ll mem;

void f(ll);

main()
{
	ll stk = VAL; // assignment to stack
	mem = VAL; // assignment to global memory

	stk = VAL + stk; // assignment to register (indirect)

	f(VAL); // assignment to argument
}
