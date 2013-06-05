// RUN: %ucc -c %s

#define VAL (1LL << 39)

typedef long long ll;

ll mem;

main()
{
	ll stk = VAL; // assignment to stack
	mem = VAL; // assignment to global memory

	stk = VAL + stk; // assignment to register (indirect)
}
