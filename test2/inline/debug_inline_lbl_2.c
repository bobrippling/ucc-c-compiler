// RUN: %ucc -g %s

__attribute((always_inline))
g(int *a, int *b)
{
}

__attribute((always_inline))
f(int x)
{
	int local = 3;

	if(1||x){
		int local2 = 6;

		g(&local, local2);

		__builtin_trap();
	}else{
		return local;
	}

	// unreachable code - Ldbg_end.* should still be emitted
}

main()
{
	return f(2);
}
