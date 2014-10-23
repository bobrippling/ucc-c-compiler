// RUN: %ucc -g %s

g()
{
}

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
